/*
	ATIRE.C
	-------
*/
#include <stdio.h>
#include <string.h>
#include "atire_api.h"
#include "str.h"
#include "maths.h"
#include "ant_param_block.h"
#include "stats_time.h"
#include "channel_file.h"
#include "channel_socket.h"
#include "relevance_feedback_factory.h"

#ifndef FALSE
	#define FALSE 0
#endif
#ifndef TRUE
	#define TRUE (!FALSE)
#endif

const char *PROMPT = "]";
const long MAX_TITLE_LENGTH = 1024;

ATIRE_API *atire = NULL;

ATIRE_API * ant_init(ANT_ANT_param_block & params);

/*
	PERFORM_QUERY()
	---------------
*/
double perform_query(long topic_id, ANT_channel *outchannel, ANT_ANT_param_block *params, char *query, long long *matching_documents)
{
ANT_stats_time stats;
char message[1024];
long long now, search_time;

/*
	Search
*/
now = stats.start_timer();
*matching_documents = atire->search(query, params->sort_top_k, params->query_type);
search_time = stats.stop_timer(now);

/*
	Report
*/
if (params->stats & ANT_ANT_param_block::SHORT)
	{
	if (topic_id >= 0)
		{
		sprintf(message, "Topic:%ld ", topic_id);
		outchannel->puts(message);
		}
	sprintf(message, "<query>%s</query><numhits>%lld</numhits><time>%lld</time>", query, *matching_documents, stats.time_to_milliseconds(search_time));
	outchannel->puts(message);
	}

if (params->stats & ANT_ANT_param_block::QUERY)
	atire->stats_text_render();

/*
	Return average precision
*/
return atire->get_whole_document_precision(topic_id, params->metric, params->metric_n);
}

/*
	PROMPT()
	--------
*/
void prompt(ANT_ANT_param_block *params)
{
if (params->queries_filename == NULL && params->port == 0)		// coming from stdin
	printf(PROMPT);
}

/*
	BETWEEN()
	---------
*/
char *between(char *source, char *open_tag, char *close_tag)
{
char *start,*finish;

if ((start = strstr(source, open_tag)) == NULL)
	return NULL;

start += strlen(open_tag);

if ((finish = strstr(start, close_tag)) == NULL)
	return NULL;

return strnnew(start, finish - start);
}

/*
	ANT()
	-----
*/
double ant(ANT_ANT_param_block *params)
{
char *print_buffer, *ch, *pos;
ANT_stats_time post_processing_stats;
char *command, *query;
long topic_id, line, number_of_queries;
long long hits, result, last_to_list, first_to_list;
double average_precision, sum_of_average_precisions, mean_average_precision, relevance;
long length_of_longest_document;
unsigned long current_document_length;
long long docid;
char *document_buffer, *title_start, *title_end;
ANT_channel *inchannel, *outchannel;
char **answer_list, *filename;

if (params->port == 0)
	{
	inchannel = new ANT_channel_file(params->queries_filename);		// stdin
	outchannel = new ANT_channel_file();							// stdout
	}
else
	inchannel = outchannel = new ANT_channel_socket(params->port);	// in/out to given port

print_buffer = new char [MAX_TITLE_LENGTH + 1024];

length_of_longest_document = atire->get_longest_document_length();

document_buffer = new char [length_of_longest_document + 1];

sum_of_average_precisions = 0.0;
number_of_queries = line = 0;

prompt(params);
for (command = inchannel->gets(); command != NULL; prompt(params), command = inchannel->gets())
	{
	first_to_list = 0;
	last_to_list = first_to_list + params->results_list_length;

	line++;
	/*
		Parsing to get the topic number
	*/
	strip_space_inplace(command);

	if (strcmp(command, ".loadindex") == 0)
		{
		/*
			NOTE: Do not expose this command to untrusted users as it could almost certainly
			cause arbitrary code execution by loading specially-crafted attacker-controlled indexes.
		*/
		params->set_doclist_filename(strip_space_inplace(filename = inchannel->gets()));
		delete [] filename;

		params->set_index_filename(strip_space_inplace(filename = inchannel->gets()));
		delete [] filename;

		ATIRE_API * new_api = ant_init(*params);

		if (new_api) 
			{
			delete atire;
			atire = new_api;

			length_of_longest_document = atire->get_longest_document_length();
			delete [] document_buffer;
			document_buffer = new char [length_of_longest_document + 1];

			outchannel->puts("Succeeded");
			}
		else
			{
			/* Leave global 'atire' unchanged */
			outchannel->puts("Failed");
			}
		delete [] command;
		continue;
		}
	else if (strcmp(command, ".describeindex") == 0)
		{
		delete [] command;

		outchannel->puts(params->doclist_filename);
		outchannel->puts(params->index_filename);

		continue;
		}
	else if (strcmp(command, ".quit") == 0)
		{
		delete [] command;
		break;
		}
	else if (strncmp(command, ".get ", 5) == 0)
		{
		*document_buffer = '\0';
		if ((current_document_length = length_of_longest_document) != 0)
			{
			atire->get_document(document_buffer, &current_document_length, atoll(command + 5));
			sprintf(print_buffer, "%lld", current_document_length);
			outchannel->puts(print_buffer);
			outchannel->write(document_buffer, current_document_length);
			}
		delete [] command;
		continue;
		}
	else if (strncmp(command, "<ATIREsearch>", 13) == 0)
		{
		topic_id = -1;
		if ((query = between(command, "<query>", "</query>")) == NULL)
			{
			delete [] command;
			continue;
			}

		if ((pos = strstr(command, "<top>")) != NULL)
			first_to_list = atol(pos + 5)  - 1;
		else
			first_to_list = 0;

		if ((pos = strstr(command, "<n>")) != NULL)
			last_to_list = first_to_list + atol(pos + 3);
		else
			last_to_list = first_to_list + params->results_list_length;

		delete [] command;
		command = query;
		}
	else if (strncmp(command, "<ATIREgetdoc>", 13) == 0)
		{
		*document_buffer = '\0';
		if ((current_document_length = length_of_longest_document) != 0)
			{
			atire->get_document(document_buffer, &current_document_length, atoll(strstr(command, "<docid>") + 7));
			outchannel->puts("<ATIREgetdoc>");
			sprintf(print_buffer, "<length>%lld</length>", current_document_length);
			outchannel->puts(print_buffer);
			outchannel->write(document_buffer, current_document_length);
			outchannel->puts("</ATIREgetdoc>");
			}
		delete [] command;
		continue;
		}
	else if (strncmp(command, "<ATIREloadindex>", 16) == 0)
		{
		params->set_doclist_filename(filename = between(command, "<doclist>", "</doclist>"));
		delete [] filename;

		params->set_index_filename(filename = between(command, "<index>", "</index>"));
		delete [] filename;

		ATIRE_API * new_api = ant_init(*params);

		if (new_api) 
			{
			delete atire;
			atire = new_api;

			length_of_longest_document = atire->get_longest_document_length();
			delete [] document_buffer;
			document_buffer = new char [length_of_longest_document + 1];

			outchannel->puts("<ATIREloadindex>1</ATIREloadindex>");
			}
		else 
			{
			/* Leave global 'atire' unchanged */
			outchannel->puts("<ATIREloadindex>0</ATIREloadindex>");
			}

		delete [] command;
		continue;
		}
	else if (strncmp(command, "<ATIREdescribeindex>", strlen("<ATIREdescribeindex>")) == 0)
		{
		delete [] command;

		outchannel->puts("<ATIREdescribeindex>");

		outchannel->write("<doclist filename=\"");
		outchannel->write(params->doclist_filename);
		outchannel->puts("\"/>");

		outchannel->write("<index filename=\"");
		outchannel->write(params->index_filename);
		outchannel->puts("\"/>");
		outchannel->puts("</ATIREdescribeindex>");

		continue;
		}
	else if (*command == '\0')
		{
		delete [] command;
		continue;			// ignore blank lines
		}
	else if (params->assessments_filename != NULL || params->output_forum != ANT_ANT_param_block::NONE || params->queries_filename != NULL)
		{
		topic_id = atol(command);
		if ((query = strchr(command, ' ')) == NULL)
			exit(printf("Line %ld: Can't process query as badly formed:'%s'\n", line, command));
		}
	else
		{
		topic_id = -1;
		query = command;
		}

	outchannel->puts("<ATIREsearch>");
	/*
		Do the query and compute average precision
	*/
	number_of_queries++;
	average_precision = perform_query(topic_id, outchannel, params, query, &hits);
	sum_of_average_precisions += average_precision;		// zero if we're using a focused metric

	/*
		Report the average precision for the query
	*/
	if (params->assessments_filename != NULL && params->stats & ANT_ANT_param_block::SHORT)
		printf("Topic:%ld Average Precision:%f\n", topic_id , average_precision);

	/*
		How many results to display on the screen.
	*/
	if (first_to_list > hits)
		first_to_list = last_to_list = hits;
	if (first_to_list < 0)
		first_to_list = 0;
	if (last_to_list > hits)
		last_to_list = hits;
	if (last_to_list < 0)
		last_to_list = 0;
	/*
		Convert from a results list into a list of documents and then display (or write to the forum file)
	*/
	if (params->output_forum != ANT_ANT_param_block::NONE)
		atire->write_to_forum_file(topic_id);
	else
		{
		answer_list = atire->generate_results_list();

		outchannel->puts("<hits>");
		for (result = first_to_list; result < last_to_list; result++)
			{
			docid = atire->get_relevant_document_details(result, &docid, &relevance);
			if ((current_document_length = length_of_longest_document) == 0)
				title_start = "";
			else
				{
				/*
					Get the title of the document (this is a bad hack and should be removed)
				*/
				atire->get_document(document_buffer, &current_document_length, docid);
				if ((title_start = strstr(document_buffer, "<title>")) == NULL)
					if ((title_start = strstr(document_buffer, "<TITLE>")) == NULL)
						title_start = "";
				if (*title_start != '\0')
					{
					title_start += 7;
					if ((title_end = strstr(title_start, "</title>")) == NULL)
						title_end = strstr(title_start, "</TITLE>");
					if (title_end != NULL)
						{
						if (title_end - title_start > MAX_TITLE_LENGTH)
							title_end = title_start + MAX_TITLE_LENGTH;
						*title_end = '\0';
						for (ch = title_start; *ch != '\0'; ch++)
							if (!ANT_isprint(*ch))
								*ch = ' ';
						}
					}
				}
			sprintf(print_buffer, "<hit><rank>%lld</rank><id>%lld</id><name>%s</name><rsv>%0.2f</rsv><title>%s</title></hit>", result + 1, docid, answer_list[result], relevance, title_start);
			outchannel->puts(print_buffer);
			}
		outchannel->puts("</hits>");
		}
	outchannel->puts("</ATIREsearch>");
	delete [] command;
	}

/*
	delete the document buffer
*/
delete [] document_buffer;

/*
	Compute Mean Average Precision
*/
mean_average_precision = sum_of_average_precisions / (double)number_of_queries;

/*
	Report MAP
*/
if (params->assessments_filename != NULL && params->stats & ANT_ANT_param_block::PRECISION)
	printf("\nProcessed %ld topics (MAP:%f)\n\n", number_of_queries, mean_average_precision);

/*
	Report the summary of the stats
*/
if (params->stats & ANT_ANT_param_block::SUM)
	atire->stats_all_text_render();

/*
	Clean up
*/
delete inchannel;
if (inchannel != outchannel)
	delete outchannel;
delete [] print_buffer;

/*
	And finally report MAP
*/
return mean_average_precision;
}

/*
	ANT_INIT()
	----------

	Create and return a new API and search engine using the given parameters.

	If the search engine fails to load the index from disk (e.g. sharing violation),
	return NULL.
*/
ATIRE_API * ant_init(ANT_ANT_param_block & params)
{

/* Instead of overwriting the global API, create a new one and return it.
 * This way, if loading the index fails, we can still use the old one.
 */
ATIRE_API * atire = new ATIRE_API();
int fail;

if (params.logo)
	puts(atire->version());				// print the version string is we parsed the parameters OK

if (params.ranking_function == ANT_ANT_param_block::READABLE)
	fail = atire->open(ANT_ANT_param_block::READABLE | params.file_or_memory, params.index_filename, params.doclist_filename);
else
	fail = atire->open(params.file_or_memory, params.index_filename, params.doclist_filename);

if (fail) 
	{
	delete atire;

	return NULL;
	}

if (params.assessments_filename != NULL)
	atire->load_assessments(params.assessments_filename);

if (params.output_forum != ANT_ANT_param_block::NONE)
	atire->set_forum(params.output_forum, params.output_filename, params.participant_id, params.run_name, params.results_list_length);

atire->set_trim_postings_k(params.trim_postings_k);
atire->set_stemmer(params.stemmer, params.stemmer_similarity, params.stemmer_similarity_threshold);
atire->set_feedbacker(params.feedbacker, params.feedback_documents, params.feedback_terms);

atire->set_segmentation(params.segmentation);
switch (params.ranking_function)
	{
	case ANT_indexer_param_block_rank::BM25:
		atire->set_ranking_function(params.ranking_function, params.bm25_k1, params.bm25_b);
		break;
	case ANT_indexer_param_block_rank::LMD:
		atire->set_ranking_function(params.ranking_function, params.lmd_u, 0.0);
		break;
	case ANT_indexer_param_block_rank::LMJM:
		atire->set_ranking_function(params.ranking_function, params.lmjm_l, 0.0);
		break;
	case ANT_indexer_param_block_rank::KBTFIDF:
		atire->set_ranking_function(params.ranking_function, params.kbtfidf_k, params.kbtfidf_b);
		break;
	default:
		atire->set_ranking_function(params.ranking_function, 0.0, 0.0);
	}

return atire;
}

/*
	MAIN()
	------
*/
int main(int argc, char *argv[])
{
ANT_stats stats;
ANT_ANT_param_block params(argc, argv);

params.parse();

atire = ant_init(params);

if (!atire)
	{
	return -1;
	}

ant(&params);

delete atire;

printf("Total elapsed time including startup and shutdown ");
stats.print_elapsed_time();
ANT_stats::print_operating_system_process_time();
return 0;
}