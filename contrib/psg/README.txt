Mon Jan 26 16:00 CEST 2015
This is version 1.0 of PeerSimGrid

1) An overview
2) Preliminaries
3) Compile & Run 

--------------------------------------------------------------------------------------------
1) An overview:
PeerSimGrid (PSG) is an interface developed in Java and allows users to simulate 
and execute their code under PeerSim or Simgrid simulator, using PeerSim implementation policy.
	
This archive is composed of:
* the src/ directory containing the simulator source and examples
* the configs/ directory containing example of configuration files
* psg.jar, a java archive containing all libraries  


2) Preliminaries:
	Before using psg simulator, you need to make some changes in your configuration file:
		* Replace the "UniformRandomTransport" transport protocol by "psgsim.PSGTransport".
		* Replace the "simulation.endtime" by "simulation.duration"
		* you can define your platform on the configuration file as:
			platform path/to/your/file.xml 
	
	
3) Compile & Run:
	In short, the way to compile and execute your code is: 
		Compile it: 		   
		$> make compile
		
		Test it (optional):
		$> make test
 		This test execute two examples, (chord and edaggregation found in the example folder), under the two simulators
		and compare their outputs.	
		
		Run it:
		$> ./run.sh path/to/your/configuration_file 
		For example: 
			$>./run.sh configs/chordPSG.txt 
			
		For the documentation
		$> make doc
		
		For the help command
		$> make help
		
Note: For more informations please contact khaled.baati@gmail.com