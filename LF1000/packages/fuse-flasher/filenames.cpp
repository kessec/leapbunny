#include "filenames.h"
#include "fuse-flasher.h"

#include <list>

static std::list<const char *> gPathsSeen;

// Return 1 if found, 0 if not
int seen_path(const char *path)
{
	// dump_paths ();
	RETURN ("Searching for path");
	std::list<const char *>::iterator it;
	for (it=gPathsSeen.begin(); it != gPathsSeen.end(); it++)
	{
		if (strcmp (*it, path) == 0)
			return 1;
	}
	return 0;
}

// Add path to list
void add_path(const char *path)
{
	if (!seen_path (path))
	{
		char *copy = strdup (path);
		gPathsSeen.push_back(copy);
		RETURN ("Adding path");
	}
}

//Searches for and removes the path
int remove_path(const char* path)
{
	RETURN ("Removing path");
	std::list<const char *>::iterator it;
	for (it=gPathsSeen.begin(); it != gPathsSeen.end(); it++)
	{
		if (strcmp (*it, path) == 0)
		{
			gPathsSeen.erase(it);
			return 1;
		}
	}
	return 0;
}

void dump_paths()
{
	const char *path=".";
	std::list<const char *>::iterator it;
	int any=0;
	for (it=gPathsSeen.begin(); it != gPathsSeen.end(); it++)
	{
		NOTE1 ("Path='%s'", *it);
		any=1;
	}
	if (!any)
		RETURN ("No Paths yet");
}
