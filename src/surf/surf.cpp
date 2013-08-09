#include "surf.hpp"

/*********
 * Model *
 *********/

void Model::addTurnedOnCallback(ResourceCallback rc)
{
  m_resOnCB = rc;
}

void Model::notifyResourceTurnedOn(ResourcePtr r)
{
  m_resOnCB(r);
}

void Model::addTurnedOffCallback(ResourceCallback rc)
{
  m_resOffCB = rc;
}

void Model::notifyResourceTurnedOff(ResourcePtr r)
{
  m_resOffCB(r);
}

void Model::addActionCancelCallback(ActionCallback ac)
{
  m_actCancelCB = ac;
}

void Model::notifyActionCancel(ActionPtr a)
{
  m_actCancelCB(a);
}

void Model::addActionResumeCallback(ActionCallback ac)
{
  m_actResumeCB = ac;
}

void Model::notifyActionResume(ActionPtr a)
{
  m_actResumeCB(a);
}

void Model::addActionSuspendCallback(ActionCallback ac)
{
  m_actSuspendCB = ac;
}

void Model::notifyActionSuspend(ActionPtr a)
{
  m_actSuspendCB(a);
}


/************
 * Resource *
 ************/

string Resource::getName() {
  return m_name;
}

bool Resource::isOn()
{
  return m_running;
}

void Resource::turnOn()
{
  if (!m_running) {
    m_running = true;
    p_model->notifyResourceTurnedOn(this);
  }
}

void Resource::turnOff()
{
  if (m_running) {
    m_running = false;
    p_model->notifyResourceTurnedOff(this);
  }
}

/**********
 * Action *
 **********/

void Action::cancel()
{
  p_model->notifyActionCancel(this);
}

void Action::suspend()
{
  p_model->notifyActionSuspend(this);
}

void Action::resume()
{
  p_model->notifyActionResume(this);
}

bool Action::isSuspended()
{
  return false;
}

