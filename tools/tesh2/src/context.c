#include <context.h>


XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(tesh);

#define INDEFINITE_SIGNAL	NULL

context_t
context_new(void)
{
	context_t context = xbt_new0(s_context_t, 1);
	
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

context_t
context_dup(context_t context)
{
	
	context_t dup = xbt_new0(s_context_t, 1);
	
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
		dup->signal = strdup(context->signal);

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
context_reset(context_t context)
{
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

void
context_free(context_t* context)
{
	if(((*context)->input))
		xbt_strbuff_free(((*context)->input));

	if(((*context)->output))
		xbt_strbuff_free(((*context)->output));
	
	if((*context)->signal)
		free((*context)->signal);

	*context = NULL;
}

