/*
	MERGER_PARAM_BLOCK.H
	---------------------
*/
#ifndef MERGER_PARAM_BLOCK_H_
#define MERGER_PARAM_BLOCK_H_

#include "indexer_param_block.h"

/*
	class ANT_MERGER_PARAM_BLOCK
	-----------------------------
*/
class ANT_merger_param_block : public ANT_indexer_param_block
{
private:
	int argc;
	char **argv;

protected:
	void document_compression(char *scheme);
	void compression(char *schemes);
	void term_removal(char *list);

public:
	ANT_merger_param_block(int argc, char *argv[]);
	virtual ~ANT_merger_param_block() {}
	virtual void usage(void);
	virtual void help(void);
	virtual long parse(void);
} ;

#endif  /* MERGER_PARAM_BLOCK_H_ */
