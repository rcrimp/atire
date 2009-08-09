/*
	RANKING_FUNCTION_SIMILARITY.C
	-----------------------------
*/
#include <stdio.h>
#include <limits.h>
#include <math.h>
#include "ranking_function_similarity.h"
#include "search_engine_btree_leaf.h"
#include "compress.h"
#include "search_engine_accumulator.h"

/*
	ANT_RANKING_FUNCTION_SIMILARITY::ANT_RANKING_FUNCTION_SIMILARITY()
	------------------------------------------------------------------
*/
ANT_ranking_function_similarity::ANT_ranking_function_similarity(ANT_search_engine *engine, double k1, double b) : ANT_ranking_function(engine)
{
const double one_minus_b = 1.0 - b;
long current;

this->k1 = k1;
this->b = b;

/*
	Precompute the length normalisation prior probability part of BM25
*/
document_prior_probability = new float [(size_t)documents_as_integer];
for (current = 0; current < documents_as_integer; current++)
	document_prior_probability[current] =  (float)(k1 * (one_minus_b + b * (document_lengths[current] / mean_document_length)));
}

/*
	ANT_RANKING_FUNCTION_SIMILARITY::~ANT_RANKING_FUNCTION_SIMILARITY()
	-------------------------------------------------------------------
*/
ANT_ranking_function_similarity::~ANT_ranking_function_similarity()
{
delete [] document_prior_probability;
}

/*
	ANT_RANKING_FUNCTION_SIMILARITY::RELEVANCE_RANK_TOP_K()
	-------------------------------------------------
*/
void ANT_ranking_function_similarity::relevance_rank_top_k(ANT_search_engine_accumulator *accumulator, ANT_search_engine_btree_leaf *term_details, ANT_compressable_integer *impact_ordering, long long trim_point)
{
#ifdef ANT_BM25_SLOW
	const double b = this->b;
	const double one_minus_b = 1.0 - b;
#endif
const double k1 = this->k1;
const double k1_plus_1 = k1 + 1.0;
long docid;
double top_row, tf, idf;
ANT_compressable_integer *current, *end;

/*
	          N
	IDF = log -
	          n

	This variant of IDF is better than log((N - n + 0.5) / (n + 0.5)) on the 70 INEX 2008 Wikipedia topics
*/
idf = log((double)documents / (double)term_details->document_frequency);

/*
	               tf(td) * (k1 + 1)
	rsv = ----------------------------------- * IDF
	                                  len(d)
	      tf(td) + k1 * (1 - b + b * --------)
                                    av_len_d

	In this implementation we ignore k3 and the number of times the term occurs in the query.
*/
/*
	Alright, so, we have the trim_point at this point.  The trim point is an optimising factor.  It is the
	number of postings we should process before early-terminating.  As the postings are stored in impact
	order (typically from highest to lowest term frequency), the first posting is most probably the most
	relevant document and so on.  Oh, but as there will be multiple postings with the same impact score
	and we don't know which is "best", the early termination actually only happens at the end of processing
	one complete impact value's postings list.

	So how do we compute where to end?  The impacted list is of length:document frequencies plus the impacts 
	plus the terminators.  So we can compute the ending point from document frequency if we add to that pointer
	for each impact and each zero.  Now that we can compare document frequency, we can compare to the trim_point
	measured in postings and early terminate after we have processed that "batch" of postings.
*/
current = impact_ordering;
end = impact_ordering + (term_details->document_frequency >= trim_point ? trim_point : term_details->document_frequency);
while (current < end)
	{
	end += 2;		// account for the impact_order and the terminator
	tf = *current++;
	top_row = tf * k1_plus_1;
	docid = -1;
	while (*current != 0)
		{
		docid += *current++;
#ifdef ANT_BM25_SLOW
		/*
			This version uses the document lengths from the search_engine object
			which takes more time.  The fast version is preferred as it is a lot laster
		*/
		accumulator[docid].add_rsv(idf * (top_row / (tf + k1 * (one_minus_b + b * (document_lengths[docid] / mean_document_length)))));
#else
		/*
			This version uses the document prior probabilities computed in the constructor
			which takes more memory
		*/
		accumulator[docid].add_rsv(idf * (top_row / (tf + document_prior_probability[docid])));
#endif
		}
	current++;		// skip over the zero
	}
}

/*
	ANT_RANKING_FUNCTION_SIMILARITY::RELEVANCE_RANK_TF()
	----------------------------------------------------
	This is the variant that gets called for stemming.
*/
void ANT_ranking_function_similarity::relevance_rank_tf(ANT_search_engine_accumulator *accumulator, ANT_search_engine_btree_leaf *term_details, ANT_weighted_tf *tf_array, long long trim_point)
{
const double k1 = this->k1;
const double b = this->b;
const double k1_plus_1 = k1 + 1.0;
const double one_minus_b = 1.0 - b;
double tf, idf, top_row;
ANT_weighted_tf *current, *end;
long docid;

compute_term_details(term_details, tf_array);	// get document_frequency;

idf = log((double)(documents) / (double)term_details->document_frequency);

current = tf_array;
end = tf_array + (term_details->document_frequency >= trim_point ? trim_point : term_details->document_frequency);

for (current = tf_array; current < end; current++)
	if (*current != 0)
		{
        tf = *current >= 0x100 ? 0xFF : *current;
		docid = current - tf_array;
		top_row = tf * k1_plus_1;

		accumulator[docid].add_rsv(idf * (top_row / (tf + k1 * (one_minus_b + b * (document_lengths[docid] / mean_document_length)))));
		}

relevance_rank_top_k(accumulator, term_details, decompress_buffer, trim_point);
}
