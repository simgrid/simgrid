#include <suite.h>
#include <unit.h>

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(tesh);

suite_t
suite_new(unit_t owner, const char* description)
{
	suite_t suite = (suite_t)malloc (sizeof (s_suite_t));
	
	suite->number = 0;
	suite->units = vector_new(DEFAULT_UNITS_CAPACITY, unit_free);
	suite->owner = owner;
	suite->description = description;
	suite->number_of_successed_units = 0;
	suite->number_of_failed_units = 0;
	
	return suite;
}


void
suite_include_unit(suite_t suite, unit_t unit)
{
	vector_push_back(suite->units, unit);
	suite->number++;
}

void
suite_free(suite_t* suite)
{
	if(*suite)
	{
		vector_free(&((*suite)->units));
		free(*suite);
		*suite = NULL;
	}
}

