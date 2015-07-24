package example.symphony;

/**
 *
 * @author Andrea Esposito <and1989@gmail.com>
 */
public class Tuple<X, Y> {

    public X x;
    public Y y;

    public Tuple() {
    }

    public Tuple(X x, Y y) {
        this.x = x;
        this.y = y;
    }

    @Override
    public boolean equals(Object obj) {

        if (obj instanceof Tuple) {
            Tuple tuple = (Tuple) obj;

            // (x != null && tuple.x != null) ==> (x==tuple.x || x.equals(tuple.x))
            // x == null <==> tuple.x == null

            boolean equalsX = (x == null && tuple.x == null) || ((x != null && tuple.x != null) && (x == tuple.x || x.equals(tuple.x)));
            boolean equalsY = (y == null && tuple.y == null) || ((y != null && tuple.y != null) && (y == tuple.y || y.equals(tuple.y)));

            return equalsX && equalsY;
        }

        return false;
    }

    @Override
    public int hashCode() {
        int hash = 5;
        hash = 89 * hash + (this.x != null ? this.x.hashCode() : 0);
        hash = 89 * hash + (this.y != null ? this.y.hashCode() : 0);
        return hash;
    }
}
