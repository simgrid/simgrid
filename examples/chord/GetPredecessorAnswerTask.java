package chord;

public class GetPredecessorAnswerTask extends ChordTask {
	public int answerId;
	public GetPredecessorAnswerTask(String issuerHostname, String answerTo, int answerId) {
		super(issuerHostname,answerTo);
		this.answerId = answerId;
	}
}
