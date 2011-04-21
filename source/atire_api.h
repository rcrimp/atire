/*
	ATIRE_API.H
	-----------
*/
#ifndef ATIRE_API_H_
#define ATIRE_API_H_

#include <limits.h>

class ANT_NEXI_ant;
class ANT_NEXI_term_ant;
class ANT_search_engine;
class ANT_stemmer;
class ANT_query_boolean;
class ANT_query;
class ANT_ranking_function;
class ANT_memory;
class ANT_memory_index;
class ANT_memory_index_one;
class ANT_relevant_document;
class ANT_mean_average_precision;
class ANT_search_engine_forum;
class ANT_assessment_factory;
class ANT_bitstring;
class ANT_query_parse_tree;
class ANT_string_pair;
class ANT_relevance_feedback;

/*
	class ATIRE_API
	---------------
*/
class ATIRE_API
{
public:
	enum { INDEX_IN_FILE = 0, INDEX_IN_MEMORY = 1, READABILITY_SEARCH_ENGINE = 2 } ;
	enum { QUERY_NEXI = 1, QUERY_BOOLEAN = 2, QUERY_FEEDBACK = 4 } ;

private:
	char token_buffer[1024];				// used to convert parsed string_pairs into C char * strings.
	ANT_memory *memory;						// ATIRE memory allocation scheme

	ANT_NEXI_ant *NEXI_parser;				// INEX CO / CAS queries
	ANT_query_boolean *boolean_parser;		// full boolean
	long segmentation;						// Chinese segmentation algorithm
	ANT_query *parsed_query;				// the parsed query
	ANT_search_engine *search_engine;		// the search engine itself
	ANT_ranking_function *ranking_function;	// the ranking function to use (default is the perameterless Divergence From Randomness)
	ANT_stemmer *stemmer;					// stemming function to use
	ANT_relevance_feedback *feedbacker;		// relevance feedback algorithm to use (NULL = none)
	long feedback_documents;				// documents to analyse in relevance feedback
	long feedback_terms;					// terms (extracted from top documents) to use in relevance feedback
	long query_type_is_all_terms;			// use the DISJUNCTIVE ranker but only find documents containing all of the search terms (CONJUNCTIVE)
	long long hits;							// how many documents were found at the last query
	long long sort_top_k;					// ranking is only accurate to this position in the results list

	char **document_list;					// list (in order) of the external IDs of the documents in the collection
	char **filename_list;					// the same list, but assuming filenames (parsed for INEX)
	char **answer_list;						// 
	long long documents_in_id_list;			// the length of the above two lists (the number of docs in the collection)
	char *mem1, *mem2;						// arrays of memory holding the above;

	ANT_assessment_factory *assessment_factory;		// the machinery to read different formats of assessments (INEX and TREC)
	ANT_relevant_document *assessments;		// assessments for measuring percision (at TREC and INEX)
	long long number_of_assessments;		// length of the assessments array
	ANT_mean_average_precision *map;		// the object that computes MAP scores
	ANT_search_engine_forum *forum_writer;	// the object that writes the results list in the INEX or TREC format
	long forum_results_list_length;			// maximum length of a results list for the evaluation form (INEX or TREC)

protected:
	char **read_docid_list(char * doclist_filename, long long *documents_in_id_list, char ***filename_list, char **mem1, char **mem2);
	static char *max(char *a, char *b, char *c);
	long process_NEXI_query(char *query);
	ANT_bitstring *process_boolean_query(ANT_query_parse_tree *root, long *leaves);
	void boolean_to_NEXI(ANT_NEXI_term_ant *into, ANT_query_parse_tree *root, long *leaves);
	long process_NEXI_query(ANT_NEXI_term_ant *parse_tree);
	long process_boolean_query(char *query);
	char *string_pair_to_term(char *destination, ANT_string_pair *source, size_t destination_length, long case_fold = 0);
	void query_object_with_feedback_to_NEXI_query(void);

public:
	ATIRE_API();
	virtual ~ATIRE_API();

	/*
		What version are we?
	*/
	char *version(long *version_number = 0);

	/*
		Load all the necessary stuff for the search engine to start up
		This assumes we are in same directory as the index
	*/
	long open(long type, char * index_filename = "index.aspt", char * doclist_filename = "doclist.aspt");		// see the enum above for possible types (ORed together)

	/*
		Load an assessment file (for INEX or TREC)
	*/
	long load_assessments(char *assessments_filename);

	/*
		Set the chinese segmentation algorithm
	*/
	void set_segmentation(long segmentation) { this->segmentation = segmentation; }

	/*
		Parse a NEXI query
	*/
	long parse_NEXI_query(char *query);												// returns an error code (0 = no error, see query.h)

	/*
		Set the ranking function
		for BM25: k1 = p1, b = k2
		for LMD:  u = p1
		for LMJM: l = p1
	*/
	long set_ranking_function(long function, double p1, double p2);

	/*
		Set the stemming function
	*/
	long set_stemmer(long which_stemmer, long stemmer_similarity, double threshold);

	/*
		Set the relevance feedback mechanism
	*/
	long set_feedbacker(long feedbacker, long documents, long terms);

	/*
		Set the static pruning point.  At most sttic_prune_point postings will be read from disk and processedS
	*/
	long long set_trim_postings_k(long long static_prune_point);

	/*
		Given the query, do the seach, rank, and return the number of hits
		query_type is either QUERY_NEXI or QUERY_BOOLEAN
	*/
	long long search(char *query, long long top_k = LLONG_MAX, long query_type = QUERY_NEXI);

	/*
		Call the re-ranker.  This takes the top few results and re-orders based on
		analysis of the result set.
	*/
	void rerank(void);

	/*
		Turn the numeric internal IDs into a list of external string IDs (post search)
	*/
	char **generate_results_list(void);

	/*
		Given a positing in the results list return the internal search engine docid and its relevance
	*/
	long long get_relevant_document_details(long long result, long long *docid, double *relevance);

	/*
		What is the average precision of the query we've just done?
	*/
	double get_whole_document_precision(long query_id, long metric, long metric_n);

	/*
		Configre TREC or INEX output format
	*/
	long set_forum(long type, char *output_filename, char *participant_id, char *run_name, long forum_results_list_length);

	/*
		Write the results out in INEX or TREC format (as specified by set_form)
	*/
	void write_to_forum_file(long topic_id);

	/*
		Get the length of the longest document in the repository
		useful so that you can allocate a buffer for get_document.
	*/
	long get_longest_document_length(void);

	/*
		Load a document from the repository (if there is one)
	*/
	char *get_document(char *buffer, unsigned long *length, long long id);

	/*
		Rendering of statistics to do with the last query
	*/
	void stats_text_render(void);

	/*
		Rendering of statistics to do with all queries so far since the search engine started
	*/
	void stats_all_text_render(void);
} ;

#endif /* ATIRE_API_H_ */