package chord;

public class FindSuccessorTask extends ChordTask {
	public int requestId;
	
	public FindSuccessorTask(String issuerHostname, String answerTo,  int requestId) {
		super(issuerHostname, answerTo);
		this.requestId = requestId;
	}
}
