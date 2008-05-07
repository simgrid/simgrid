/*
 * src/context.c - type representing the tesh file stream.
 *
 * Copyright 2008,2009 Martin Quinson, Malek Cherier All right reserved. 
 *
 * This program is free software; you can redistribute it and/or modify it 
 * under the terms of the license (GNU LGPL) which comes with this package.
 *
 * Purpose:
 * 		This file contains all the definitions of the functions related with
 * 		the tesh context type.
 *
 */
 
#include <context.h>


XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(tesh);

#define INDEFINITE_SIGNAL	NULL

context_t
context_new(void)
{
	context_t context = xbt_new0(s_context_t,1);
	
	context->line = NULL;
	context->command_line = NULL;
	context->exit_code = 0;
	context->timeout = INDEFINITE;
	context->input = xbt_strbuff_new();
	context->output = xbt_strbuff_new();
	context->signal = INDEFINITE_SIGNAL;
	context->output_handling = oh_check;
	context->async = 0;
	
	return context;
}

int
context_free(context_t* ptr)
{
	/* TODO : check the parameter */
	if(((*ptr)->input))
		xbt_strbuff_free(((*ptr)->input));

	if(((*ptr)->output))
		xbt_strbuff_free(((*ptr)->output));
	
	if((*ptr)->signal)
		free((*ptr)->signal);

	*ptr = NULL;
	
	return 0;
}

int
context_reset(context_t context)
{
	
	/* TODO : check the parameter */
	
	context->line = NULL;
	context->command_line = NULL;

	if(context->input)
		xbt_strbuff_empty(context->input);

	if(context->output)
		xbt_strbuff_empty(context->output);
	
	if(context->signal)
	{
		free(context->signal);
		context->signal = NULL;
	}
	
	/* default expected exit code */
	context->exit_code = 0;

	context->output_handling = oh_check;
	context->async = 0;
	
	return 0;

}

context_t
context_dup(context_t context)
{
	
	context_t dup;
	
	/* TODO : check the parameter */
	
	dup = xbt_new0(s_context_t, 1);
	
	dup->line = context->line;
	dup->command_line = context->command_line;
	dup->exit_code = context->exit_code;
	dup->timeout = context->timeout;
	dup->output = NULL;
	dup->input = NULL;
	dup->signal = NULL;
	
	if(context->input->used)
	{
		dup->input = xbt_strbuff_new();
		xbt_strbuff_append(dup->input,context->input->data);
	}

	if(context->output->used)
	{
		dup->output = xbt_strbuff_new();
		xbt_strbuff_append(dup->output,context->output->data);
	}

	if(context->signal)
	{
		if(!(dup->signal = strdup(context->signal)))
		{
			free(dup);
			return NULL;
		}
	}

	dup->output_handling = context->output_handling;
	
	dup->async = context->async;
	
	return dup;
}

void
context_clear(context_t context)
{
	context->line = NULL;
	context->command_line = NULL;
	context->exit_code = 0;
	context->timeout = INDEFINITE;
	
	if(context->input)
		xbt_strbuff_empty(context->input);

	if(context->output)
		xbt_strbuff_empty(context->output);
	
	if(context->signal)
	{
		free(context->signal);
		context->signal = INDEFINITE_SIGNAL;
	}

	context->output_handling = oh_check;
	context->async = 0;

}

void
context_input_write(context_t context, const char* buffer)
{
	xbt_strbuff_append(context->input, buffer);
}

void
context_ouput_read(context_t context, const char* buffer)
{
	xbt_strbuff_append(context->output, buffer);
}



