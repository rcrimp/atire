/*
	DIRECTORY_ITERATOR_MULTIPLE.C
	-----------------------------
*/
#include <stdio.h>
#include <string.h>
#include "str.h"
#include "producer_consumer.h"
#include "directory_iterator_multiple.h"
#include "directory_iterator_multiple_internals.h"
#include "stats.h"
#include "memory.h"

#ifndef FALSE
	#define FALSE 0
#endif
#ifndef TRUE
	#define TRUE (!FALSE)
#endif

long ANT_directory_iterator_multiple::tid = 0;

/*
	ANT_DIRECTORY_ITERATOR_MULTIPLE::ANT_DIRECTORY_ITERATOR_MULTIPLE()
	------------------------------------------------------------------
*/
ANT_directory_iterator_multiple::ANT_directory_iterator_multiple(long get_file) : ANT_directory_iterator("", get_file)
{
sources_length = sources_used = 0;
sources = NULL;
thread_details = NULL;
producer = NULL;

clock = new ANT_stats(new ANT_memory);

message = new char[50];
sprintf(message, "multiple %ld ", ANT_directory_iterator_multiple::tid);
}

/*
	ANT_DIRECTORY_ITERATOR_MULTIPLE::~ANT_DIRECTORY_ITERATOR_MULTIPLE()
	-------------------------------------------------------------------
*/
ANT_directory_iterator_multiple::~ANT_directory_iterator_multiple()
{
long current;

for (current = 0; current < sources_used; current++)
	delete sources[current];

delete [] sources;
delete [] thread_details;
delete producer;
}

/*
	ANT_DIRECTORY_ITERATOR_MULTIPLE::ADD_ITERATOR()
	-----------------------------------------------
*/
void ANT_directory_iterator_multiple::add_iterator(ANT_directory_iterator *iterator)
{
ANT_directory_iterator **memory;

if (sources_used >= sources_length)
	{
	sources_length += sources_growth_factor;
	memory = new ANT_directory_iterator *[sources_length];
	memcpy(memory, sources, sources_used * sizeof(ANT_directory_iterator *));
	delete [] sources;
	sources = memory;

	delete [] thread_details;
	thread_details = new ANT_directory_iterator_multiple_internals[sources_length];
	}
sources[sources_used] = iterator;
sources_used++;
}

/*
	ANT_DIRECTORY_ITERATOR_MULTIPLE::PRODUCE()
	------------------------------------------
	This is the producer in the classic producer / consumer model
*/
void ANT_directory_iterator_multiple::produce(ANT_directory_iterator_multiple_internals *my)
{
ANT_directory_iterator_object *got;
char *message = new char[50];

sprintf(message, "multiple %ld ", ANT_directory_iterator_multiple::tid++);

//printf("%sstart_upstream %lld\n", message, clock->start_timer());

/*
	Exhaust the supply
*/
//printf("%sstart_wait %lld\n", message, clock->start_timer());
for (got = my->iterator->first(&my->file_object); got != NULL; got = my->iterator->next(&my->file_object))
	producer->add(&my->file_object);

//printf("%send_wait %lld\n", message, clock->start_timer());
/*
	At this point the thread is finished, but we have the remaining problem of
	signaling back to the consumer that all the producers are done.

	The obvious way to do this is to shove an empty element into the list
	and to count them at the consumer's end
*/
my->file_object.filename = NULL;
producer->add(&my->file_object);
//printf("%send_upstream %lld\n", message, clock->start_timer());

/*
	Now the thread is done it'll die its natural death
*/
}

/*
	ANT_DIRECTORY_ITERATOR_MULTIPLE::BOOTSTRAP()
	--------------------------------------------
*/
void *ANT_directory_iterator_multiple::bootstrap(void *param)
{
ANT_directory_iterator_multiple_internals *current;

current = (ANT_directory_iterator_multiple_internals *)param;
current->thread_id = ANT_thread_id();

current->parent->produce(current);

return NULL;
}

/*
	ANT_DIRECTORY_ITERATOR_MULTIPLE::FIRST()
	----------------------------------------
*/
ANT_directory_iterator_object *ANT_directory_iterator_multiple::first(ANT_directory_iterator_object *object)
{
long instance;
ANT_directory_iterator_multiple_internals *current;

START;

/*
	Set up this object
*/
producer = new ANT_producer_consumer<ANT_directory_iterator_object> (sources_used);
active_threads = sources_used;

/*
	Start each thread
*/
instance = 0;
for (current = thread_details; current < thread_details + sources_used; current++)
	{
	memset(&current->file_object, 0, sizeof(current->file_object));
	current->iterator = sources[instance];
	current->parent = this;
	current->get_file = get_file;
	current->instance = instance++;
	current->files_read = 0;
	ANT_thread(bootstrap, current);
	}

END;

/*
	And then consume
*/
return next(object);
}

/*
	ANT_DIRECTORY_ITERATOR_MULTIPLE::NEXT()
	---------------------------------------
	This is the consumer in the classic producer / consumer model
*/
ANT_directory_iterator_object *ANT_directory_iterator_multiple::next(ANT_directory_iterator_object *object)
{
long finished = FALSE;

mutex.enter();
START;
	if (active_threads <= 0)
		finished = TRUE;
	else
		do
			{
			producer->remove(object);
			if (object->filename != NULL)
				break;
			else
				if (--active_threads <= 0)
					finished = TRUE;				// all sources have dried up
			}
		while (!finished);
END;
mutex.leave();

if (finished)
	return NULL;
else
	return object;
}

