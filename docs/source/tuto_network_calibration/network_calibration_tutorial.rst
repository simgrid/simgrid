SimGrid MPI calibration of a Grid5000 cluster
*********************************************

This tutorial demonstrates how to properly calibrate SimGrid to reflect
the performance of MPI operations in a Grid’5000 cluster. However, the same
approach can be performed to calibrate any other environment.

This tutorial is the result of the effort from many people along the years.
Specially, it is based on Tom Cornebize’s Phd thesis
(https://tel.archives-ouvertes.fr/tel-03328956).

You can execute the notebook `network_calibration_tutorial.ipynb <https://framagit.org/simgrid/simgrid/tree/master/docs/source/tuto_network_calibration/network_calibration_tutorial.ipynb>`_) by yourself using the docker image
available at: `Dockerfile <https://framagit.org/simgrid/simgrid/tree/master/docs/source/tuto_network_calibration/Dockerfile>`_. For that, run the
following commands in the tutorial folder inside simgrid's code source (``docs/source/tuto_network_calibration``):

.. code-block::

    docker build -t tuto_network .
    docker run -p 8888:8888 tuto_network

0. Introduction
===============

Performing a realistic simulation is hard and therefore the correct SimGrid calibration requires some work.

We briefly present these steps here and detail some of them later. Please, refer to the different links and the original notebook
for more details.

1. **Execution of tests in a real platform**

  Executing the calibration code in a real platform to obtain the raw data
  to be analyzed and inject in SimGrid.

2. **MPI Async/Sync modes: Identifying threshold**

  Identify the threshold of the asynchronous and synchronous mode of MPI.

3. **Segmentation**

  Identify the semantic breakpoints of each MPI operation.

4. **Clustering**

  Aggregating the points inside each segment to create noise models.
  
  In this tutorial, we propose 2 alternatives to automatically do the clustering:
  *ckmeans.1d.dp* and *dhist*. You must choose one, test and maybe adapt it
  manually depending on your platform.

5. **Description of the platform in SimGrid**

  Writing your platform file using the models created by this notebook.

6. **SimGrid execution and comparison**

  Re-executing the calibration code in SimGrid and comparing the simulation and real world.

*This tutorial focuses on steps 3 to 6. For other steps, please see the
available links.*

1. Execution of tests in a real platform
========================================

The first step is running tests in a real platform to obtain the data to be used in the calibration.

The platform-calibration project provides a tool to run MPI experiments. In a few words, the tool will run a
bunch of MPI operations in the nodes to gather their performance. In this tutorial, we are interested in 4 measures
related to network operations:

- **MPI_Send**: measures the time spent in blocking MPI_Send command.
- **MPI_Isend**: same for non-blocking MPI_Isend command.
- **MPI_Recv**: time spent in MPI_Recv.
- **Ping-pong**: measures the elapsed time to perform a MPI_Send followed by a MPI_Recv.

The first 3 tests (MPI_Send, MPI_Isend and MPI_Recv) are used to calibrate the SMPI options
(:ref:`smpi/os<cfg=smpi/os>`, :ref:`smpi/or<cfg=smpi/or>`, :ref:`smpi/ois<cfg=smpi/ois>`) while
the Ping-pong is used for network calibration (:ref:`network/latency-factor<cfg=network/latency-factor>`
and :ref:`network/bandwidth-factor<cfg=network/bandwidth-factor>`).

For more detail about this step, please refer to:
https://framagit.org/simgrid/platform-calibration

The result of this phase can be seen in the figure below. These are the results for the
calibration on Grid'5000 dahu cluster at Grenoble/France.

.. image:: /tuto_network_calibration/plot_op_raw.png


We can see a huge variability in the measured elapsed time for each MPI operation, specially:

- **Performance leaps**: at some points, MPI changes its operation mode and the duration can increase drastically.
  This is mainly due to the different implementation of the MPI.
- **Noise/variability**: for a same message size, we have different elapsed times, forming the horizontal lines you can see in the figure. 

In order to do a correct simulation, we must be able to identify and model these different phenomena.


2. MPI Async/Sync modes: Identifying threshold
==============================================

MPI communications can operate in different modes
(asynchronous/synchronous), depending on the message size of your
communication. In asynchronous mode, the MPI_Send will return
immediately while in synchronous it’ll wait for respective MPI_Recv
starts before returning. See Section `2.2
SimGrid/SMPI <https://tel.archives-ouvertes.fr/tel-03328956/document>`__
for more details.

The first step is identifying the message size from which MPI starts
operating in synchronous mode. This is important to determine which
dataset to use in further tests (individual MPI_Send/MPI_Recv or
PingPong operations).

In this example, we set the threshold to **63305**, because it’s the data
available in our tests and matches the output of the segmentation tool.

However the real threshold for this platform is 64000. To be
able to identify it, another study would be necessary and the adjustment
of the breakpoints needs to be made. We refer to the Section `5.3.2
Finding semantic
breakpoints <https://tel.archives-ouvertes.fr/tel-03328956/document>`__
for more details.


3. Segmentation
===============

The objective of the segmentation phase is identify the **performance leaps** in MPI operations.
The first step for segmentation is removing the noise by averaging the duration for each message size.

.. image:: /tuto_network_calibration/plot_op_average.png

Visually, you can already identify some of the segments (e.g. around 1e5 for MPI_Isend).


However, we use a tool `pycewise <https://github.com/Ezibenroc/pycewise>`_ that makes this job and finds the correct vertical lines which divide each segment.

We present here a summarized version of the results for MPI_Send and Ping-Pong operations. For detailed version, please see "Segmentation" section in `network_calibration_tutorial.ipynb <https://framagit.org/simgrid/simgrid/tree/master/docs/source/tuto_network_calibration/network_calibration_tutorial.ipynb>`_.

**MPI_Send**

.. raw:: html

    <div>
    <style scoped>
        .dataframe tbody tr th:only-of-type {
            vertical-align: middle;
        }
    
        .dataframe tbody tr th {
            vertical-align: top;
        }
    
        .dataframe thead th {
            text-align: right;
        }
    </style>
    <table border="1" class="dataframe">
      <thead>
        <tr style="text-align: right;">
          <th></th>
          <th>min_x</th>
          <th>max_x</th>
          <th>intercept</th>
          <th>coefficient</th>
        </tr>
      </thead>
      <tbody>
        <tr>
          <th>0</th>
          <td>-inf</td>
          <td>8.0</td>
          <td>2.064276e-07</td>
          <td>6.785879e-09</td>
        </tr>
        <tr>
          <th>1</th>
          <td>8.0</td>
          <td>4778.0</td>
          <td>3.126291e-07</td>
          <td>7.794590e-11</td>
        </tr>
        <tr>
          <th>2</th>
          <td>4778.0</td>
          <td>8133.0</td>
          <td>7.346840e-40</td>
          <td>1.458088e-10</td>
        </tr>
        <tr>
          <th>3</th>
          <td>8133.0</td>
          <td>33956.0</td>
          <td>4.052195e-06</td>
          <td>1.042737e-10</td>
        </tr>
        <tr>
          <th>4</th>
          <td>33956.0</td>
          <td>63305.0</td>
          <td>8.556209e-06</td>
          <td>1.262608e-10</td>
        </tr>
      </tbody>
    </table>
    </div>

|

This is the example of the pycewise's output for **MPI_Send** operation. Each line represents one segment which is characterized by:

- **interval** (min_x, max_x): the message size interval for this segment
- **intercept**: output of the *linear model* of this segment
- **coefficient**: output of the *linear model* of this segment

The average duration of each segment is characterized by the formula: :math:`coefficient*msg\_size + intercept`.

**Ping-pong**

In the ping-pong case, we are interested only in the synchronous mode, so we keep the segments
with message size greater than 65503.

.. raw:: html

    <div>
    <style scoped>
        .dataframe tbody tr th:only-of-type {
            vertical-align: middle;
        }
    
        .dataframe tbody tr th {
            vertical-align: top;
        }
    
        .dataframe thead th {
            text-align: right;
        }
    </style>
    <table border="1" class="dataframe">
      <thead>
        <tr style="text-align: right;">
          <th></th>
          <th>min_x</th>
          <th>max_x</th>
          <th>intercept</th>
          <th>coefficient</th>
        </tr>
      </thead>
      <tbody>
        <tr>
          <th>4</th>
          <td>63305.0</td>
          <td>inf</td>
          <td>0.000026</td>
          <td>1.621952e-10</td>
        </tr>
      </tbody>
    </table>
    </div>

|

**Setting the base bandwidth and latency for our platform**

We use the ping-pong results to estimate the bandwidth and latency for
our dahu cluster. These values are passed to SimGrid in the JSON files 
and are used later to calculate network factors.

To obtain similar timing in SimGrid simulations, your platform must use
these values when describing the links.

In this case, the hosts in dahu are interconnected through
a single link with this bandwidth and latency.

.. code:: python

    bandwidth_base = (1.0/reg_pingpong_df.iloc[0]["coefficient"])*2.0
    latency_base = reg_pingpong_df.iloc[0]['intercept']/2.0
    print("Bandwidth: %e" % bandwidth_base)
    print("Latency: %e" % latency_base)


.. parsed-literal::

    Bandwidth: 1.233082e+10
    Latency: 1.292490e-05


3.1. Segmentation results
-------------------------

The figure below presents the results of the segmentation phase for the dahu calibration.

At this phase, you may need to adjust the segments and select those to keep. You can for example
do the union of the different segments for each MPI operation to keep them uniform.

For simplicity, we do nothing in this tutorial.

.. image:: /tuto_network_calibration/plot_op_segmented.png

The linear models are sufficient to emulate the average duration of each operation.

However, you may be interested in a more realistic model capable of generating the noise and variability for each message size.

For that, it's necessary the clustering phase to create specific models for the noise inside each segment.

4. Clustering
=============

We present 2 tool options for creating the noise models for MPI
communications: **ckmeans** and **dhist**.

You probably want to try both and see which one is better in your
environment. Note that a manual tuning of the results may be needed.

The output of the clustering phase is injected in SimGrid. To make this
easier, we export the different models using JSON files.

Again, we present here just a few results to illustrate the process. For complete information, please see "Clustering" section in `network_calibration_tutorial.ipynb <https://framagit.org/simgrid/simgrid/tree/master/docs/source/tuto_network_calibration/network_calibration_tutorial.ipynb>`_. Also, you can check the 2 individual notebooks that are used for the clustering: `clustering_ckmeans.ipynb <https://framagit.org/simgrid/simgrid/tree/master/docs/source/tuto_network_calibration/clustering_ckmeans.ipynb>`_ and `clustering_dhist.ipynb <https://framagit.org/simgrid/simgrid/tree/master/docs/source/tuto_network_calibration/clustering_dhist.ipynb>`_.

4.1. Ckmeans.1d.dp (alternative 1)
----------------------------------

The noise is modeled here by a mixture of normal distributions. For each
segmented found by pycewise, we have a set of normal distributions (with
their respective probabilities) that describes the noise.

Ckmeans is used to aggregate the points together. One mixture of normal
distributions is created for each cluster.

.. image:: /tuto_network_calibration/plot_ckmeans_PingPong.png
   :scale: 25%

The figure above presents the output for ping-pong. The process involves 4 phases:

1. **Quantile regression**: a quantile regression is made to have our baseline linear model. A quantile regression is used to avoid having negative intercepts and consequently negative estimate duration times.
2. **Intercept residuals**: from the quantile regression, we calculate the intercept for each message size (:math:`intercept = duration - coefficient*msg\_size`)
3. **Ckmeans**: creates a set of groups based on our intercept residuals. In the figure, each color represents a group.
4. **Normal distributions**: for each group found by ckmeans, we calculate the mean and standard deviation of that group. The probabilities are drawn from the density of each group (points in group/total number of points).


**Ping-pong**


Ping-pong measures give us the round-trip estimated time, but we need
the elapsed time in 1 direction to inject in SimGrid.

For simplicity, we just scale down the normal distributions.
However, a proper calculation may be necessary at this step.

.. code:: python

    pingpong_models["coefficient"] = pingpong_models["coefficient"]/2
    pingpong_models["mean"] = pingpong_models["mean"]/2
    pingpong_models["sd"] = pingpong_models["sd"]/numpy.sqrt(2)
    pingpong_models


.. raw:: html

    <div>
    <style scoped>
        .dataframe tbody tr th:only-of-type {
            vertical-align: middle;
        }
    
        .dataframe tbody tr th {
            vertical-align: top;
        }
    
        .dataframe thead th {
            text-align: right;
        }
    </style>
    <table border="1" class="dataframe">
      <thead>
        <tr style="text-align: right;">
          <th></th>
          <th>mean</th>
          <th>sd</th>
          <th>prob</th>
          <th>coefficient</th>
          <th>min_x</th>
          <th>max_x</th>
        </tr>
      </thead>
      <tbody>
        <tr>
          <th>0</th>
          <td>0.000012</td>
          <td>4.356809e-07</td>
          <td>0.499706</td>
          <td>8.049632e-11</td>
          <td>63305.0</td>
          <td>3.402823e+38</td>
        </tr>
        <tr>
          <th>1</th>
          <td>0.000013</td>
          <td>5.219426e-07</td>
          <td>0.385196</td>
          <td>8.049632e-11</td>
          <td>63305.0</td>
          <td>3.402823e+38</td>
        </tr>
        <tr>
          <th>2</th>
          <td>0.000019</td>
          <td>1.673437e-06</td>
          <td>0.073314</td>
          <td>8.049632e-11</td>
          <td>63305.0</td>
          <td>3.402823e+38</td>
        </tr>
        <tr>
          <th>3</th>
          <td>0.000025</td>
          <td>2.023256e-06</td>
          <td>0.024108</td>
          <td>8.049632e-11</td>
          <td>63305.0</td>
          <td>3.402823e+38</td>
        </tr>
        <tr>
          <th>4</th>
          <td>0.000030</td>
          <td>2.530620e-06</td>
          <td>0.011696</td>
          <td>8.049632e-11</td>
          <td>63305.0</td>
          <td>3.402823e+38</td>
        </tr>
        <tr>
          <th>5</th>
          <td>0.000037</td>
          <td>3.533823e-06</td>
          <td>0.005980</td>
          <td>8.049632e-11</td>
          <td>63305.0</td>
          <td>3.402823e+38</td>
        </tr>
      </tbody>
    </table>
    </div>

|

This table presents the clustering results for Ping-pong. Each line represents a normal distribution that characterizes the noise along with its probability.

At our simulator, we'll draw our noise following these probabilities/distributions.


Finally, we dump the results in a JSON format. Below, we present the `pingpong_ckmeans.json <https://framagit.org/simgrid/simgrid/tree/master/docs/source/tuto_network_calibration/pingpong_ckmeans.json>`_ file.

This file will be read by your simulator later to generate the proper factor for network operations.

.. parsed-literal::

    {'bandwidth_base': 12330818795.43382,
     'latency_base': 1.2924904864614219e-05,
     'seg': [{'mean': 1.1503128856516448e-05,
       'sd': 4.3568091437319533e-07,
       'prob': 0.49970588235294106,
       'coefficient': 8.04963230919345e-11,
       'min_x': 63305.0,
       'max_x': 3.4028234663852886e+38},
      {'mean': 1.2504551284320949e-05,
       'sd': 5.219425841751762e-07,
       'prob': 0.385196078431373,
       'coefficient': 8.04963230919345e-11,
       'min_x': 63305.0,
       'max_x': 3.4028234663852886e+38},
      {'mean': 1.879472592512515e-05,
       'sd': 1.6734369316865939e-06,
       'prob': 0.0733137254901961,
       'coefficient': 8.04963230919345e-11,
       'min_x': 63305.0,
       'max_x': 3.4028234663852886e+38},
      {'mean': 2.451754075327485e-05,
       'sd': 2.0232563328989863e-06,
       'prob': 0.0241078431372549,
       'coefficient': 8.04963230919345e-11,
       'min_x': 63305.0,
       'max_x': 3.4028234663852886e+38},
      {'mean': 3.004149952883e-05,
       'sd': 2.5306204869242285e-06,
       'prob': 0.0116960784313725,
       'coefficient': 8.04963230919345e-11,
       'min_x': 63305.0,
       'max_x': 3.4028234663852886e+38},
      {'mean': 3.688584189653765e-05,
       'sd': 3.5338234385210185e-06,
       'prob': 0.00598039215686275,
       'coefficient': 8.04963230919345e-11,
       'min_x': 63305.0,
       'max_x': 3.4028234663852886e+38}]}


The same is done for each one of the MPI operations, creating the different input files: `pingpong_ckmeans.json <https://framagit.org/simgrid/simgrid/tree/master/docs/source/tuto_network_calibration/pingpong_ckmeans.json>`_, `isend_ckmeans.json <https://framagit.org/simgrid/simgrid/tree/master/docs/source/tuto_network_calibration/isend_ckmeans.json>`_, `recv_ckmeans.json <https://framagit.org/simgrid/simgrid/tree/master/docs/source/tuto_network_calibration/recv_ckmeans.json>`_,  `send_ckmeans.json <https://framagit.org/simgrid/simgrid/tree/master/docs/source/tuto_network_calibration/send_ckmeans.json>`_.


4.2. Dhist (alternative 2)
--------------------------

Alternatively, we can model the noise using non-uniform histograms.

Diagonally cut histograms are used in this case, one histogram for each
segment.

The noise is later sampled according to these histograms.

Note: For better results, we had to apply a log function on the elapsed
time before running the dhist algorithm. However, it’s not clear why
this manipulation gives better results.

.. image:: /tuto_network_calibration/plot_dhist_PingPong.png
   :scale: 25%

The figure presents the histogram for the ping-pong operation.

In the x-axis, we have the intercept residuals calculated using the linear models found by pycewise.

The vertical lines are the bins found by dhist. Note that the size of each bin varies depending on their density.

**Ping-pong**

Ping-pong measures give us the round-trip estimated time, but we need
the elapsed time in 1 direction to inject in SimGrid. As we applied the log function on our data, we need a minor trick to calculate the elapsed time.

:math:`\frac{e^x}{2}` = :math:`e^{x + log(\frac{1}{2})}`

.. code:: python

    for i in pingpong_dhist:
        i["xbr"] = [v + numpy.log(1/2) for v in i["xbr"]]
        i["coeff"] /= 2
    
    pingpong_dhist = {"bandwidth_base": bandwidth_base, "latency_base" : latency_base, "seg": pingpong_dhist}
    pingpong_dhist


.. parsed-literal::

    {'bandwidth_base': 12330818795.43382,
     'latency_base': 1.2924904864614219e-05,
     'seg': [{'log': True,
       'min_x': 63305.0,
       'max_x': 3.4028234663852886e+38,
       'xbr': [-11.541562041539144,
        -11.441125408005446,
        -11.400596947874545,
        -11.372392420653046,
        -11.341231770713947,
        -11.306064060041345,
        -11.262313043898645,
        -11.167260850740746,
        -11.054191810141747,
        -10.945733341460246,
        -10.851269918507747,
        -10.748196672490847,
        -10.639355545006445,
        -10.532059052445776,
        -10.421953284283596,
        -10.311044865949563,
        -10.199305798019065,
        -10.086544751090685,
        -9.973069718835006],
       'height': [28047.5350565562,
        386265.096035713,
        648676.945998964,
        566809.701663792,
        477810.03815685294,
        342030.173378546,
        41775.283991878,
        972.856932519077,
        10123.6907854913,
        43371.2845877054,
        21848.5405963759,
        9334.7066819517,
        12553.998437911001,
        6766.22135638404,
        5166.42477286285,
        3535.0214326622204,
        1560.8226847324402,
        202.687759084986],
       'coeff': 8.10976153806028e-11}]}

This JSON file is read by the simulator to create the platform and generate the appropriate noise.
The same is done for each one of the MPI operations, creating the different input files: `pingpong_dhist.json <https://framagit.org/simgrid/simgrid/tree/master/docs/source/tuto_network_calibration/pingpong_dhist.json>`_, `isend_dhist.json <https://framagit.org/simgrid/simgrid/tree/master/docs/source/tuto_network_calibration/isend_dhist.json>`_, `recv_dhist.json <https://framagit.org/simgrid/simgrid/tree/master/docs/source/tuto_network_calibration/recv_dhist.json>`_, `send_dhist.json <https://framagit.org/simgrid/simgrid/tree/master/docs/source/tuto_network_calibration/send_dhist.json>`_.

5. Description of the platform in SimGrid
=========================================

At this point we have done the analysis and extracted the models in the several JSON files. It's possible now to create our platform file that will be used by SimGrid later.

The platform is created using the C++ interface from SimGrid. The result is a library file (.so) which is loaded by SimGrid when running the application.

The best to understand is reading the C++ code in `docs/source/tuto_network_calibration <https://framagit.org/simgrid/simgrid/tree/master/docs/source/tuto_network_calibration/>`_, the main files are:

- `dahu_platform_ckmeans.cpp <https://framagit.org/simgrid/simgrid/tree/master/docs/source/tuto_network_calibration/dahu_platform_ckmeans.cpp>`_: create the dahu platform using the JSON files from ckmeans.
- `dahu_platform_dhist.cpp <https://framagit.org/simgrid/simgrid/tree/master/docs/source/tuto_network_calibration/dahu_platform_dhist.cpp>`_: same for dhist output.
- `Utils.cpp <https://framagit.org/simgrid/simgrid/tree/master/docs/source/tuto_network_calibration/Utils.cpp>`_/`Utils.hpp <https://framagit.org/simgrid/simgrid/tree/master/docs/source/tuto_network_calibration/Utils.hpp>`_: some auxiliary classes used by both platforms to handle the segmentation and sampling.
- `CMakeLists.txt <https://framagit.org/simgrid/simgrid/tree/master/docs/source/tuto_network_calibration/CMakeLists.txt>`_: create the shared library to be loaded by SimGrid

Feel free to re-use and adapt these files according to your needs.

6. SimGrid execution and comparison
===================================

6.1. Execution
--------------

**Ckmeans.1d.dp** and **Dhist**

The execution is similar for both modes. The only change is the platform library to be used: **libdahu_ckmeans.so** or **libdhist.so**.


.. code:: bash

    %%bash
    
    cd /source/simgrid.git/docs/source/tuto_network_calibration/
    
    smpirun --cfg=smpi/simulate-computation:0 \
        --cfg=smpi/display-timing:yes \
        -platform ./libdahu_ckmeans.so \
        -hostfile /tmp/host.txt -np 2 \
        /source/platform-calibration/src/calibration/calibrate -d /tmp/exp -m 1 -M 1000000 -p exp -s /tmp/exp.csv


.. parsed-literal::

    Read bandwidth_base: 1.233082e+10 latency_base: 1.292490e-05
    Starting parsing file: pingpong_ckmeans.json
    Starting parsing file: send_ckmeans.json
    Starting parsing file: isend_ckmeans.json
    Starting parsing file: recv_ckmeans.json
    [0] MPI initialized
    [0] nb_exp=115200, largest_size=980284
    [0] Alloc size: 1960568 
    [1] MPI initialized
    [1] nb_exp=115200, largest_size=980284
    [1] Alloc size: 1960568 
    [0.000000] [xbt_cfg/INFO] Configuration change: Set 'smpi/privatization' to '1'
    [0.000000] [xbt_cfg/INFO] Configuration change: Set 'smpi/np' to '2'
    [0.000000] [xbt_cfg/INFO] Configuration change: Set 'smpi/hostfile' to '/tmp/host.txt'
    [0.000000] [xbt_cfg/INFO] Configuration change: Set 'surf/precision' to '1e-9'
    [0.000000] [xbt_cfg/INFO] Configuration change: Set 'network/model' to 'SMPI'
    [0.000000] [xbt_cfg/INFO] Configuration change: Set 'smpi/simulate-computation' to '0'
    [0.000000] [xbt_cfg/INFO] Configuration change: Set 'smpi/display-timing' to 'yes'
    [0.000000] [xbt_cfg/INFO] Configuration change: Set 'smpi/tmpdir' to '/tmp'
    [0.000000] [smpi_config/INFO] You did not set the power of the host running the simulation.  The timings will certainly not be accurate.  Use the option "--cfg=smpi/host-speed:<flops>" to set its value.  Check https://simgrid.org/doc/latest/Configuring_SimGrid.html#automatic-benchmarking-of-smpi-code for more information.
    [6.845963] [smpi_utils/INFO] Simulated time: 6.84596 seconds. 
    
    The simulation took 71.6111 seconds (after parsing and platform setup)
    1.77771 seconds were actual computation of the application



6.2. Comparison
---------------

Finally, let’s compare the SimGrid results the real ones. The red points are the real data while the blue ones are the output from our simulator.

**Ckmeans.1d.dp**

.. image:: /tuto_network_calibration/plot_op_simgrid_ckmeans.png

**Dhist**

.. image:: /tuto_network_calibration/plot_op_simgrid_dhist.png


**Ping-Pong**

Note that for ping-ping tests, we have an important gap between the real
performance (in red) and SimGrid (in blue) for messages below our
sync/async threshold (63305).

This behavior is explained by how we measure the extra cost for each
MPI_Send/MPI_Recv operations.

In `calibrate.c <https://framagit.org/simgrid/platform-calibration/-/blob/master/src/calibration/calibrate.c>`_ in platform-calibration, the ping-pong test is as follows
(considering the processes are synchronized):


.. image:: /tuto_network_calibration/fig/pingpong_real.png

We can see that we measure the delay at **Process 1**, just before the
first *MPI_Send-1* until the end of respective *MPI_Recv-2*. Moreover,
the extra cost of MPI operations is paid concurrently with the network
communication cost.

In this case, it doesn't matter when the *MPI_Send-2* will finish.
Despite we expect that it finished before the *MPI_Recv-2*, we couldn't
be sure.

Also, both processes are running in parallel, so we can expect that the
measure time will be:
:math:`max(\text{MPI_Send-1}, \text{MPI_Recv-1}) + \text{MPI_Recv-2}` -
:math:`max(\text{MPI_Send-1}, \text{MPI_Recv-1})`: since we cannot start
*MPI_Recv-2* or *MPI_Send_2* before finishing both commands -
:math:`\text{MPI_Recv-2}`: because we measure just after the finishing
of this receive

However, the simulation world is a little more stable. The same
communication occurs in the following way:


.. image:: /tuto_network_calibration/fig/pingpong_simgrid.png

In SimGrid, the extra costs are paid sequentially. That means, initially
we pay the extra cost for *MPI_Send-1*, after the network communication
cost, followed by the extra cost for *MPI-Recv-1*.

This effect leads to a total time of: *MPI_Send-1* + *MPI_Recv-1* +
*MPI_Send-2* + *MPI_Recv-2* which is slightly higher than the real cost.

The same doesn't happen for largest messages because we don’t pay the
extra overhead cost for each MPI operation (the communication is limited
by the network capacity).

