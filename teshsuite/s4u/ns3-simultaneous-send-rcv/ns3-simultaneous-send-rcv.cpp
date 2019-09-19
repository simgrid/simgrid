#include "simgrid/s4u.hpp"

XBT_LOG_NEW_DEFAULT_CATEGORY(s4u_test, "Messages specific for this s4u tests");

int payload = 1000;
int nb_message_to_send = 100;
double sleep_time = 1;
int nb_sender = 50;

int nb_messages_sent = 0;

simgrid::s4u::Mailbox* box;

static void test_send(){

  int nb_message = 0;

  while(nb_message < nb_message_to_send){
    
    nb_messages_sent++;
    nb_message++;
    int id_message = nb_messages_sent;
    XBT_INFO("start sending test #%i", id_message); 
    box->put(new int(id_message), payload);
    XBT_INFO("done sending test #%i", id_message);
    simgrid::s4u::this_actor::sleep_for(sleep_time);
  }
}

static void test_receive(){

  int nb_message = 0;

  while(nb_message < nb_message_to_send * nb_sender){
    XBT_INFO("waiting for messages");
    int id = *(int*)(box->get());
    nb_message++;
    XBT_INFO("received messages #%i", id);
  }
}


int main(int argc, char *argv[])
{
  simgrid::s4u::Engine e(&argc, argv);
 
  e.load_platform(argv[1]);

  for(int i=0;i<nb_sender;i++)
    simgrid::s4u::Actor::create("sender_"+std::to_string(i), e.get_all_hosts()[i], test_send);

  simgrid::s4u::ActorPtr receiver = simgrid::s4u::Actor::create("receiver", e.get_all_hosts()[nb_sender+1], test_receive);
  
  box = simgrid::s4u::Mailbox::by_name("test");
  box->set_receiver(receiver);

  e.run();

  return 0;
}
