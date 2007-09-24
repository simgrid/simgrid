package simgrid.msg;

public class Sem { 
	private int permits_;
	
	public Sem(int i) {
		permits_ = i;
	}
	
	public void acquire() throws InterruptedException {
		
		if (Thread.interrupted()) 
			throw new InterruptedException();
		
		synchronized(this) {
		
			try {
					while (permits_ <= 0) 
						wait();
					
					--permits_;
			}
			catch (InterruptedException ex) {
				notify();
				throw ex;
			}
		}
	}

	public synchronized void release() {
		++(this.permits_);
		notify();
	}
}
