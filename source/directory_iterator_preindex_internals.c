/*
	DIRECTORY_ITERATOR_PREINDEX_INTERNALS.C
	---------------------------------------
*/
#include "parser.h"
#include "parser_readability.h"
#include "memory_index_one.h"
#include "readability_factory.h"
#include "directory_iterator_preindex_internals.h"
#include "indexer_param_block.h"
#include "stemmer_factory.h"
#include "stemmer.h"
#include "directory_iterator_preindex.h"

/*
	ANT_DIRECTORY_ITERATOR_PREINDEX_INTERNALS::ANT_DIRECTORY_ITERATOR_PREINDEX_INTERNALS()
	--------------------------------------------------------------------------------------
*/
ANT_directory_iterator_preindex_internals::ANT_directory_iterator_preindex_internals(ANT_directory_iterator_preindex *parent)
{
this->parent = parent;

segmentation = parent->param_block->segmentation;

if (parent->param_block->readability_measure == ANT_readability_factory::NONE)
	parser = new ANT_parser(parent->param_block->segmentation);
else
	parser = new ANT_parser_readability();

readability = new ANT_readability_factory;
readability->set_measure(parent->param_block->readability_measure);
readability->set_parser(parser);

stemmer = ANT_stemmer_factory::get_core_stemmer(parent->param_block->stemmer);
}

/*
	ANT_DIRECTORY_ITERATOR_PREINDEX_INTERNALS::~ANT_DIRECTORY_ITERATOR_PREINDEX_INTERNALS()
	---------------------------------------------------------------------------------------
*/
ANT_directory_iterator_preindex_internals::~ANT_directory_iterator_preindex_internals()
{
delete parser;
delete readability;
delete stemmer;
}

