#include <variable.h>
 XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(tesh);
variable_t  variable_new(const char *name, const char *val) 
{
  variable_t variable;
  if (!name)
     {
    errno = EINVAL;
    return NULL;
    }
  variable = xbt_new0(s_variable_t, 1);
  variable->name = strdup(name);
  if (val)
    variable->val = strdup(val);
  variable->used = 0;
  variable->env = 0;
  variable->err = 0;
  return variable;
}

int  variable_free(variable_t * variableptr) 
{
  if (!(*variableptr))
    return EINVAL;
  if ((*((variable_t *) (variableptr)))->name)
    free((*((variable_t *) (variableptr)))->name);
  if ((*((variable_t *) (variableptr)))->val)
    free((*((variable_t *) (variableptr)))->val);
  free(*variableptr);
  *variableptr = NULL;
  return 0;
}

int  variable_is_used(variable_t variable) 
{
  if (!variable)
     {
    errno = EINVAL;
    return 0;
    }
  return variable->used;
}

int  variable_set_used(variable_t variable) 
{
  if (!variable)
    return EINVAL;
  variable->used = 1;
  return 0;
}


