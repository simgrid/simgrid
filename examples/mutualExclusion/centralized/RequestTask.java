import simgrid.msg.Task;

public class RequestTask extends Task {
	String from;
	public RequestTask(String name) {
		super();
		from=name;
	}
}
