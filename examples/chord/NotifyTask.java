package chord;

public class NotifyTask extends ChordTask {
	public int requestId;
	public NotifyTask(String issuerHostname, String answerTo, int requestId) {
		super(issuerHostname, answerTo);
		this.requestId = requestId;
	}
}
