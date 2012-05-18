package chord;

public class FindSuccessorAnswerTask extends ChordTask {
	public int answerId;

	public FindSuccessorAnswerTask(String issuerHostname, String answerTo, int answerId) {
		super(issuerHostname,answerTo);
		this.answerId = answerId;
	}
}
