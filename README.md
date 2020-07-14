# WTF-PAD

![DISCLAIMER](https://upload.wikimedia.org/wikipedia/commons/thumb/d/d7/Dialog-warning-orange.svg/40px-Dialog-warning-orange.svg.png "experimental")    **experimental - PLEASE BE CAREFUL. Intended for reasearch purposes only.**

Source code to simulate WTF-PAD on a set of traces and reproduce the results of the ESORICS 2016 paper:

```
Toward an Efficient Website Fingerprinting Defense
M. Juarez, M. Imani, M. Perry, C. Diaz, M. Wright
In the European Symposium on Research in Computer Security (ESORICS), vol. 2, pp. 27-46. Springer, 2016.
```
<!---
 - Article page in the [publisher site](https://link.springer.com/chapter/10.1007/978-3-319-45744-4_2).
--->
 - Download a [pre-print version](http://homes.esat.kuleuven.be/~mjuarezm/index_files/pdf/esorics16.pdf) for open access.

## Quick reference

```
$ python src/main.py -h

usage: main.py [-h] [-c <config name>] [--log <log path>]
               [--log-level <log level>]
               <traces path>
[...]
```

 1. Clone the repo: `git clone git@github.com:wtfpad/wtfpad.git`

 1. Install requirements: `pip install -r requirements.txt`

 1. Run a simulation with a specific configuration: `python src/main.py -c <config name> <traces path>`

 1. Check the results in `results/<config name>_<timestamp>` directory.


## Reproduce results

First of all, you will need to download the compressed data from the releases tab, place it in the `data/` directory and unpack the zipped files.

### Attack accuracy

First, run the k-NN attack on the original traces:

```
 $ ./src/knn/run_attack.sh data/closed-world-original/

 Accuracy:
 0.90000
```

Then, run the k-NN attack on the protected traces:

```
 $ ./src/knn/run_attack.sh data/closed-world-protected/

 Accuracy:
 0.199091
```

As percentages, these are:

 * Accuracy original: ~**90%**
 * Accuracy protected: ~**20%**


### Performance overhead

You can run the `overheads.py` script on the two sets of traces, the original
and the simulated ones. This script will return the latency and bandwidth
overheads.

```
 $ python src/overheads.py data/*

 Bandwidth overhead: 1.77498638326
 Latency overhead: 1.0
```

This is the overhead computed as `f(PROTECTED_TRACES) / f(ORIGINAL_TRACES)`, which means that, if the bandwidth overhead is ~1.77, the extra percentage of bandwidth is approximately **77%**. Analogously, the latency overhead as a percentage is **0%**.

Since WTF-PAD is a statistical defense strategy, the output results may vary some percentage points across different runs of the defense.

## Simulate WTF-PAD in new traces

For that, the code assumes a specific data format and, as explained in the paper, it takes a set of histograms or statistical distributions as parameters that are used by WTF-PAD to sample padding times.

You can find examples of these parameters in the `config.ini` file in the root of the repository. In particular, we used `normal_rcv` for our experiments (this configuration is the one corresponding to the lowest accuracy of the attack in Figure 5 of the paper). However, as we explain in the limitations of the paper, these distributions are fitted to the specific network conditions of the link that was used to collect the data. For WTF-PAD to run in the ideal conditions, you need to estimate these distributions for your data.

We left as future work to find an optimal set of distributions automatically and we are currently working on that. For now, we refer to the methodology in the paper to do it manually.

### Expected data format

The source code assumes that traffic trace files are represented as `<timestamp>\t<packet length>` sequences. Each line is a TCP packet as captured by `dumpcap` and the direction of the packet is encoded in the sign of the length.

The filenames of the traces have the following format: `<site index>-<instance index>`.

We used [`tor-browser-crawler`](https://github.com/webfp/tor-browser-crawler) to collect these traces.


### Configuration specification

Once you have tuned your histograms/distributions, you can encode them in a new configuration section in `config.ini`. The options that are allowed in the configuration section (e.g., `[normal]`) are:

  1. `interpolate` (type: bool, default: True): indicates whether we interpolate over the bin range in the histogram.
  1. `remove_tokens` (type: bool, default: True): indicates whether we remove a token after sampling from the histogram or put it back to its bin.
  1. `stop_on_real` (type: bool, default: True): indicates whether we transition from GAP to BURST upon receiving a real message.
  1. `percentile` (type: float, default: 0.5): tuning mechanism to adjust the trade-off between bandwidth and security.
  1. `<server|client>_<snd|rcv>_<burst|gap>_dist` (type: str, default: not specified): these are the histograms that are used at each endpoint. There is one for the BURST state and another for the GAP state. We can also define histograms for just outbound traffic (snd) or histograms that regulate padding for incoming traffic (rcv). See the paper for more details. There are two ways to specify an histogram: by passing a tuple of `<name of the distribution>, <burst probability | expected burst length>, <distribution parameters>` or `histo, <path to iat file>`. Distribution parameters are in the order expected by scipy (e.g., mean, variance for normal distribution). The iat file is a file that contains a iat (inter-arrival time) in each line (it can be generated by applying the `extract_timings.py` script on the directory with raw trafffic traces: `python src/tools/extract_timings.py data/closed-world-original`.
The `burst probability` is for the BURST histograms and the `expected burst length` is for the GAP histograms. See the `config.ini` file for examples.

Observe that since the default configuration (`[default]`) does not specify any histogram/distribution, a simulation that runs with this configuration does not add any padding and will output the original traces.

### Example using the `[normal_rcv]` configuration

```
 $ python src/main.py -c normal_rcv data/closed-world-original

 $ ./src/knn/run_attack.sh results/normal_rcv_*
   
   Accuracy:
   0.139091

 $ python src/overheads.py data/closed-world-original results/normal_rcv_*

   Bandwidth overhead: 1.94275489151
   Latency overhead: 1.00000305096

```

Run help for usage info and other options: `python src/main.py -h`:


### APE
APE is a simplified version of WTF-PAD that does not require to tune the histograms. It was developed by Tobias Pulls within the [HOT research project](https://www.cs.kau.se/pulls/hot/thebasketcase-ape/). In addition, APE is implemented as a Tor pluggable transport in [`basket2`](https://github.com/pylls/basket2), the new generation of obfuscation tools developed and maintaned by Yawning Angel. This means that APE is not simulated and is evaluated by collecting traces through a Tor that is configured to use APE as its pluggable transport.


## Questions and comments

Please, address any questions or comments to the authors of the paper. The main developers of this code are:

 - Marc Juarez (marc.juarez@kuleuven.be)
 - Mohsen Imani (imani.moh@gmail.com)

