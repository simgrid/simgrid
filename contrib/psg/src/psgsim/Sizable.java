package psgsim;

/**
 * An interface which defines a size for the message or the event to be sent. If you
 * wish that your message has a size, your class message must implements this
 * interface and defines a size parameter in the constructor.
 * 
 * @author Khaled Baati 05/02/2015
 * @version 1.1
 * 
 */
public interface Sizable {
	/**
	 * 
	 * @return The size of the message
	 */
	public double getSize();
}
