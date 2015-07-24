package example.chord;

public class FinalMessage implements ChordMessage {

	private int hopCounter = 0;

	public FinalMessage(int hopCounter) {
		this.hopCounter = hopCounter;
	}

	public int getHopCounter() {
		return hopCounter;
	}
}
