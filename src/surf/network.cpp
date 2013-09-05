#include "network.hpp"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(surfpp_network, surfpp,
                                "Logging specific to the SURF network module");

//TODO: resolve dependencies
static int TRACE_is_enabled(void) {return 0;}
static void TRACE_surf_host_set_utilization(const char *resource,
                                     const char *category,
                                     double value,
                                     double now,
                                     double delta){}
static void TRACE_surf_link_set_utilization(const char *resource,
                                     const char *category,
                                     double value,
                                     double now,
                                     double delta){}
static double TRACE_last_timestamp_to_dump = 0;

/*********
 * Utils *
 *********/

static xbt_dict_t gap_lookup = NULL;//TODO: remove static

/*********
 * Model *
 *********/

void NetworkModel::updateActionsStateLazy(double now, double delta)
{
  NetworkCm02ActionLmmPtr action;
  while ((xbt_heap_size(p_actionHeap) > 0)
         && (double_equals(xbt_heap_maxkey(p_actionHeap), now))) {
    action = (NetworkCm02ActionLmmPtr) xbt_heap_pop(p_actionHeap);
    XBT_DEBUG("Something happened to action %p", action);
#ifdef HAVE_TRACING
    if (TRACE_is_enabled()) {
      int n = lmm_get_number_of_cnst_from_var(p_maxminSystem, action->p_variable);
      unsigned int i;
      for (i = 0; i < n; i++){
        lmm_constraint_t constraint = lmm_get_cnst_from_var(p_maxminSystem,
                                                            action->p_variable,
                                                            i);
        NetworkCm02LinkPtr link = (NetworkCm02LinkPtr) lmm_constraint_id(constraint);
        TRACE_surf_link_set_utilization(link->m_name,
                                        action->p_category,
                                        (lmm_variable_getvalue(action->p_variable)*
                                            lmm_get_cnst_weight_from_var(p_maxminSystem,
                                                action->p_variable,
                                                i)),
                                        action->m_lastUpdate,
                                        now - action->m_lastUpdate);
      }
    }
#endif

    // if I am wearing a latency hat
    if (action->m_hat == LATENCY) {
      XBT_DEBUG("Latency paid for action %p. Activating", action);
      lmm_update_variable_weight(p_maxminSystem, action->p_variable, action->m_weight);
      action->heapRemove(p_actionHeap);
      action->m_lastUpdate = surf_get_clock();

        // if I am wearing a max_duration or normal hat
    } else if (action->m_hat == MAX_DURATION ||
        action->m_hat == NORMAL) {
        // no need to communicate anymore
        // assume that flows that reached max_duration have remaining of 0
      action->m_finish = surf_get_clock();
      XBT_DEBUG("Action %p finished", action);
      action->m_remains = 0;
      action->m_finish = surf_get_clock();
      action->setState(SURF_ACTION_DONE);
      action->heapRemove(p_actionHeap);

      gapRemove(action);
    }
  }
  return;
}

void NetworkModel::gapRemove(ActionLmmPtr lmm_action)
{
  xbt_fifo_t fifo;
  size_t size;
  NetworkCm02ActionLmmPtr action = (NetworkCm02ActionLmmPtr)lmm_action;

  if (sg_sender_gap > 0.0 && action->p_senderLinkName
      && action->p_senderFifoItem) {
    fifo =
        (xbt_fifo_t) xbt_dict_get_or_null(gap_lookup,
                                          action->p_senderLinkName);
    xbt_fifo_remove_item(fifo, action->p_senderFifoItem);
    size = xbt_fifo_size(fifo);
    if (size == 0) {
      xbt_fifo_free(fifo);
      xbt_dict_remove(gap_lookup, action->p_senderLinkName);
      size = xbt_dict_length(gap_lookup);
      if (size == 0) {
        xbt_dict_free(&gap_lookup);
      }
    }
  }
}

/************
 * Resource *
 ************/

/**********
 * Action *
 **********/
void NetworkCm02ActionLmm::updateRemainingLazy(double now)
{
  double delta = 0.0;

  if (m_suspended != 0)
    return;

  delta = now - m_lastUpdate;

  if (m_remains > 0) {
    XBT_DEBUG("Updating action(%p): remains was %lf, last_update was: %lf", this, m_remains, m_lastUpdate);
    double_update(&(m_remains), m_lastValue * delta);

    XBT_DEBUG("Updating action(%p): remains is now %lf", this, m_remains);
  }

  if (m_maxDuration != NO_MAX_DURATION)
    double_update(&m_maxDuration, delta);

  if (m_remains <= 0 &&
      (lmm_get_variable_weight(p_variable) > 0)) {
    m_finish = surf_get_clock();
    setState(SURF_ACTION_DONE);

    heapRemove(p_model->p_actionHeap);
  } else if (((m_maxDuration != NO_MAX_DURATION)
      && (m_maxDuration <= 0))) {
    m_finish = surf_get_clock();
    setState(SURF_ACTION_DONE);
    heapRemove(p_model->p_actionHeap);
  }

  m_lastUpdate = now;
  m_lastValue = lmm_variable_getvalue(p_variable);
}

