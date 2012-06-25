package kademlia;

/**
 * Contains the information about a foreign node according to
 * a node we are trying to find.
 */
public class Contact implements Comparable<Object> {
	private int id;
	private int distance;
	
	public Contact(int id, int distance) {
		this.id = id;
		this.distance = distance;
	}

	public int getId() {
		return id;
	}

	public int getDistance() {
		return distance;
	}
	
	public boolean equals(Object x) {
		return x.equals(id) ;
	}

	public int compareTo(Object o) {
		Contact c = (Contact)o;
		if (distance < c.distance) {
			return -1;
		}
		else if (distance == c.distance) {
			return 0;
		}
		else {
			return 1;
		}
	}

	@Override
	public String toString() {
		return "Contact [id=" + id + ", distance=" + distance + "]";
	}
	
}