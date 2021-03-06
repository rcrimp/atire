/*
	DIRECTORY_ITERATOR_TAR.H
	------------------------
*/
#ifndef DIRECTORY_ITERATOR_TAR_H_
#define DIRECTORY_ITERATOR_TAR_H_

#include "directory_iterator.h"
#include "directory_iterator_tar_file_header.h"
#include "instream.h"

/*
	class ANT_DIRECTORY_ITERATOR_TAR
	--------------------------------
*/
class ANT_directory_iterator_tar : public ANT_directory_iterator
{
public:
	enum {FULL, NAME};			// FULL filenames keep the path and the filename, NAME drops both the path and the extension

private:
	static const long TAR_BLOCK_SIZE = 512;

private:
	ANT_instream *source;
	ANT_directory_iterator_tar_file_header header;
	unsigned char buffer[TAR_BLOCK_SIZE];
	long long length_of_file_in_bytes;
	long long bytes_read;
	long filename_mode;

private:
	long detox(char *octal);
	void filename(ANT_directory_iterator_object *object);
	void read_entire_file(ANT_directory_iterator_object *object);

public:
	ANT_directory_iterator_tar(ANT_instream *source, long get_file = 0, long filename_mode = FULL) : ANT_directory_iterator("", get_file) { this->source = source; this->filename_mode = filename_mode; }
	virtual ~ANT_directory_iterator_tar() { delete source; }

	virtual ANT_directory_iterator_object *first(ANT_directory_iterator_object *object);
	virtual ANT_directory_iterator_object *next(ANT_directory_iterator_object *object);
} ;

#endif /* DIRECTORY_ITERATOR_TAR_H_ */
