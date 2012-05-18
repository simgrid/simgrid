package chord;

import org.simgrid.msg.Task;

import chord.Common;
/**
 * Base class for all Tasks in Chord.
 */
public class ChordTask extends Task {
	public String issuerHostName;
	public String answerTo;
	public ChordTask() {
		this(null,null);
	}
	public ChordTask(String issuerHostName, String answerTo) {
		super(null, Common.COMP_SIZE, Common.COMM_SIZE);
		this.issuerHostName = issuerHostName;
		this.answerTo = answerTo;
	}
}
