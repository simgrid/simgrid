import org.simgrid.s4u.*;


class Sender extends ActorMain {
  static String msg1 = "msg1= Salut mon pote";
  public void run() {
    Mailbox mb = Mailbox.by_name("hello");
    mb.put(msg1, 100000);
    System.err.println("Sender: msg1 sent");
    String msg2 = "msg2= Et la?";
    mb.put(msg2, 100000);
    System.err.println("Sender: msg2 sent; terminating");
  }
}
class Receiver extends ActorMain {
  public void run() {
    Mailbox mb = Mailbox.by_name("hello");
    Object res = mb.get();
    System.err.println("Receiver: Got1 '"+((String)res)+"'");
    Object res2 = mb.get();
    System.err.println("Receiver: Got2 '"+((String)res2)+"'"); 
  }
}

public class simgrid_runme {
  static {
    try {
      System.loadLibrary("simgrid-java");
      System.err.println("Bibliotheque chargee depuis runme");
    } catch (UnsatisfiedLinkError e) {
      System.err.println("Native code library failed to load. See the chapter on Dynamic Linking Problems in the SWIG Java documentation for help.\n" + e);
      System.exit(1);
    }
  }
  
  public static void main(String[] args) {

    Engine e = Engine.get_instance(args);
    e.load_platform("small_platform.xml");
    System.out.println("There is "+e.get_host_count()+" hosts");
    //System.out.println(e.flatify_platform());
    Actor.create("sender", e.host_by_name("Ginette"), new Sender());
    Actor.create("receiver", e.host_by_name("Fafard"), new Receiver());
    e.run();
    System.err.println("The simulation is terminating at time "+Engine.get_clock());
  }

}
