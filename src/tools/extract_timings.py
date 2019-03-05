import os
import sys
import argparse
import numpy as np
import pickle
import matplotlib.pyplot as plt
import matplotlib
matplotlib.rcParams['xtick.labelsize'] = 8

'''

Example:

Outgoing traffic, there are two bursts
      <---- --->          <--------->
      |        |          |         |
------|--------|----------|---------|-------------
      t1       t2         t3        t4


Incoming traffic, there are two bursts
                        <----------------->           <--------->
                        |                 |           |         |
------------------------|-----------------|-----------|---------|------------
                        t5                t6          t7        t8


Within these bursts the bandwidth is more than the median bandwidth of all corresponding traffic (incoming or outgoing) in our dataset.
The bandwidth is measured with a window of (2*NEIGHBORS + 1) packets whenever we receive a packet.]
IAT stands for interarrival times

client_GAP_send_histogram: IATs within a burst in outgoing traffic
server_GAP_send_histogram: IATs within a burst in incoming traffic


IAT between two bursts in the same direction
client_BURST_send_histogram:  The times like (t3 - t2)
server_BURST_send_histogram:  The times like (t7 - t6)


client_BURST_receive_histogram: the beginning of burst from server side and the starting point of burst from client side
like (t1 - t5)


server_BURST_receive_histogram: the beginning of burst from server side and the end point of burst from client side
like: (t5 - t2)


'''

CELL_LENGTH = 1.0
INCOMING = -1
OUTGOING = 1


class flow():
    def __init__(self):
        self.incoming = []
        self.outgoing = []

def calculate_bw_threshold(path_, timestamp_column=0, packet_size_column=1, delimiter='\t'):
    '''Here we calculate the median bw of all websites to use them as threshold
    we calculate two bandwidths, uplaod and download'''
    upload = []
    download = []
    traces = []

    for root, dirs, files in os.walk(path_):
        for f in files:
            #if 'b' not in f: continue #TODO this should be remove at the end
            fi = open(os.path.join(root,f),'r')
            upload_tmp = []
            download_tmp = []
            trace_tmp = flow()

            ind = -1
            for line in fi:
                ind += 1
                parts = line.split(delimiter)
                ts = float(parts[timestamp_column])
                try:
                    sz = float(parts[packet_size_column])
                except:
                    sz = float(parts[packet_size_column][:-1])
                packet = (ts, sz*float(CELL_LENGTH))
                if packet[1] > 0.0: upload_tmp.append(packet)
                if packet[1] < 0.0: download_tmp.append(packet)

                packet_ = (ts , sz*float(CELL_LENGTH), ind)# (timestamp, cell size, ind)
                if packet_[1] > 0.0: trace_tmp.outgoing.append(packet_)
                if packet_[1] < 0.0: trace_tmp.incoming.append(packet_)
            traces.append(trace_tmp)
            if (len(upload_tmp) == 0) or (len(download_tmp) == 0) or  ((upload_tmp[-1][0] - upload_tmp[0][0]) == 0.0) or ((download_tmp[-1][0] - download_tmp[0][0]) == 0.0): continue
            upload_bw_tmp = sum([abs(x[1]*float(CELL_LENGTH)) for x in upload_tmp])/(upload_tmp[-1][0] - upload_tmp[0][0])
            download_bw_tmp = sum([abs(x[1]*float(CELL_LENGTH)) for x in download_tmp])/(download_tmp[-1][0] - download_tmp[0][0])
            upload.append(upload_bw_tmp)
            download.append(download_bw_tmp)
    upload_bw = np.median(np.array(upload))
    download_bw = np.median(np.array(download))
    return upload_bw, download_bw,traces




def get_neighbors(trace, direction,num, ind):
    '''
    This function returns a list of 'num' neighbors of current position in the trace (ind) in the given direction, incoming or outgoing
    '''
    st = ind  - num
    end = ind + num +1
    if st < 0: st = 0
    if end > len(trace): end = len(trace)
    return trace[st:end]



def get_tuples_of_traces(path_):
    '''
    here we convert all the traces to list of tuples, including the iat and cell size
    '''
    traces = []
    for (root, dirs, files) in os.walk(path_):
        for f in files:
            if 'b' not in f: continue #TODO this should be remove at the end
            fi = open(os.path.join(root,f),'r')
            trace_tmp = flow()

            ind = -1
            for line in fi:
                ind += 1
                parts = line.split(delimiter)
                ts = float(parts[timestamp_column])
                try:
                    sz = float(parts[packet_size_column])
                except:
                    sz = float(parts[packet_size_column][:-1])
                packet_ = (ts , sz*float(CELL_LENGTH), ind)# (timestamp, cell size, ind)
                if packet_[1] > 0.0: trace_tmp.outgoing.append(packet_)
                if packet_[1] < 0.0: trace_tmp.incoming.append(packet_)
            traces.append(trace_tmp)

    return traces


def get_current_bw(trace, ind, num, direction):
    '''
    This function computes the current BW in position 'ind' of traces, the function gets 'num' numbers of neighbors
    in direction 'dir' and sum all cell sizes and divides them to the sum of theri iat's.
    '''
    my_neighbors = get_neighbors(trace, direction,num, ind)
    t = my_neighbors[-1][0] - my_neighbors[0][0]
    if t != 0.0:
        bw = sum([abs(x[1]) for x in my_neighbors])/t
        return bw
    else:
        return -1


def dump_file(name,data):
    for key in data:
        with open(os.path.join(name, '%s.iat' % key), 'w') as fi:
            fi.write('\n'.join(map(str, data[key])))


def extract_times(traces_path, output, neighbors, timestamp_column, packet_size_column, delimiter):
    '''
    Here we compute the probabilities and the number of bursts in traces
    '''
    if not (os.path.exists(output)):
        os.makedirs(output)

    print('\nComputing bandwidth thresholds ...\n')
    outgoing_bw_thr,incoming_bw_thr,traces = calculate_bw_threshold(traces_path, timestamp_column, packet_size_column, delimiter)  # returns upload and download bw reps #
    #outgoing_bw_thr,incoming_bw_thr = 4632.00262586, 35235.9188
    fi_ = open(os.path.join(output, 'thresholds.value'),'w')
    print(("outgoing bw threshold: {0},incoming bw threshold: {1}".format(outgoing_bw_thr,incoming_bw_thr)))
    fi_.write("outgoing bw threshold: {0},incoming bw threshold: {1}".format(outgoing_bw_thr,incoming_bw_thr))
    fi_.close()

    incoming_burst_length = []
    outgoing_burst_length = []

    client_GAP_send_histogram = []
    server_GAP_send_histogram = []

    client_BURST_send_histogram = []
    server_BURST_send_histogram = []

    server_BURST_receive_histogram = []
    client_BURST_receive_histogram = []

    start_position_in_outgoing = {}
    number_of_burst_incoming = []
    number_of_burst_outgoing = []

    trace_no = -1
    for trace_ in traces:
        trace_no += 1
        incoming_bursts = [] # in here we save the tuples of start and end points of bursts from server side
        outgoing_bursts = [] # in here we save the tuples of start and end points of bursts from client side
        # for each website
        for direction in [INCOMING, OUTGOING]:
            # for each direction
            if direction == INCOMING:
                bw_threshold = incoming_bw_thr
                trace = trace_.incoming
            else:
                bw_threshold = outgoing_bw_thr
                trace = trace_.outgoing

            burst_starting_points = []
            burst_ending_points = []
            start_burst = 0
            current_bw, prev_bw, burst_count_in_trace  = 0.0, 0.0,0
            for ind, pkt in enumerate(trace): # trace here is purely incoming or outgoing
                current_bw = get_current_bw(trace, ind, neighbors, direction)
                if current_bw == -1: current_bw = prev_bw # prev_bw should be updated


                if current_bw > bw_threshold and start_burst == 1: # Here we want to calculate and collect all iats within bursts
                    if ind-1 >= 0:
                        iat = trace[ind][0] - trace[ind-1][0]
                        if (direction == INCOMING): server_GAP_send_histogram.append(iat)#????????
                        if direction == OUTGOING:
                            client_GAP_send_histogram.append(iat)


                if current_bw > bw_threshold and start_burst == 0: # burst is going to be started
                    start_burst = 1
                    start_position_in_time = pkt[0]
                    burst_starting_points.append(start_position_in_time)
                    start_position_in_pkt_no = ind


                if current_bw < bw_threshold and start_burst == 1: # burst is going to be finished
                    start_burst = 0
                    burst_count_in_trace += 1
                    end_burst_in_time = pkt[0]
                    end_burst_in_pkt_no = ind
                    burst_ending_points.append(end_burst_in_time)
                    burst_length = end_burst_in_pkt_no - start_position_in_pkt_no + 1
                    assert (burst_length > 1.0)

                    if direction == INCOMING:
                        incoming_burst_length.append(burst_length)
                        incoming_bursts.append((start_position_in_time, end_burst_in_time))
                    if direction == OUTGOING:
                        outgoing_burst_length.append(burst_length)
                        outgoing_bursts.append((start_position_in_time, end_burst_in_time))

                if current_bw != -1: prev_bw = current_bw



            # Here we collect the number of bursts in each direction
            if direction == OUTGOING: number_of_burst_outgoing.append(burst_count_in_trace)
            if direction == INCOMING: number_of_burst_incoming.append(burst_count_in_trace)



        # Now we collect iat between two bursts in the same direction (for client BURST histogram)
        for i_,out_burst in enumerate(outgoing_bursts):
            try:
                t2 = outgoing_bursts[i_ + 1][0]
                t1 = outgoing_bursts[i_][1]
                iat = t2 - t1
                if iat >= 0.0:
                    client_BURST_send_histogram.append(iat)
            except:
                continue

        for i_,in_burst in enumerate(incoming_bursts):
            try:
                t2 = incoming_bursts[i_ + 1][0]
                t1 = incoming_bursts[i_][1]
                iat = t2 - t1
                if iat >= 0.0:
                    server_BURST_send_histogram.append(iat)
            except:
                continue



        # here we collect iat between two bursts from opposite directions
        max_tmp = 10000000000.0
        for out_burst in outgoing_bursts:
            s_B_min = max_tmp
            c_B_min = max_tmp
            for in_burst in incoming_bursts:
                t2 = in_burst[0]# the beginning of burst from server side
                t1 = out_burst[1] # the end point of burst from client side
                iat = t2 - t1

                if iat >=  0.0 and s_B_min > iat:
                    s_B_min = iat

                t1 = in_burst[0]# the beginning of burst from server side
                t2 = out_burst[0] # the starting point of burst from client side
                iat = t2 - t1

                if iat >=  0.0 and c_B_min > iat:
                    c_B_min = iat

            if c_B_min != max_tmp: client_BURST_receive_histogram.append(c_B_min)
            if s_B_min != max_tmp: server_BURST_receive_histogram.append(s_B_min)


    print("Plotting  histograms")
    print("----------------------------------------------------------------------------------")
    print("client_GAP_send_histogram: inter-arrival-times *within* bursts in *OUTGOING* traffic")
    print("server_GAP_send_histogram: inter-arrival-times *within* bursts in *INCOMING* traffic")
    print("----------------------------------------------------------------------------------")
    print("client_BURST_send_histogram: times *between* bursts in *OUTGOING* traffic")
    print("server_BURST_send_histogram: times *between* bursts in *INCOMING* traffic")
    print("----------------------------------------------------------------------------------")
    print("client_BURST_receive_histogram: times between the *beginning* of a burst from *INCOMING* traffic and the *beginning* of a burst in *OUTGOING* traffic ")
    print("server_BURST_receive_histogram: times between the *beginning* of a burst from *INCOMING* traffic and the *end* of a burst in *OUTGOING* traffic ")
    print("----------------------------------------------------------------------------------")
    bins_=np.logspace(-6.0, 1.0, 50)
    plt.hist(client_GAP_send_histogram, bins=bins_, color = 'g',alpha=0.5, label='GAP-send client ')
    plt.hist(server_GAP_send_histogram, bins=bins_, color = 'r',alpha=0.1, label='GAP-send server')
    plt.legend(loc='best')
    plt.title("GAP send histograms")
    #plt.yscale('log')
    #plt.xlim(xmax = 0.5)
    #plt.xticks(bins_)
    plt.xscale('log')
    plt.savefig(os.path.join(output,'GAP_send.pdf'), format='pdf')


    plt.figure()
    bins_=np.logspace(-3.0, 2.3, 50)
    plt.hist(client_BURST_send_histogram, bins=bins_, color = 'g', alpha=0.5, label='BURST-send client ')
    plt.hist(server_BURST_send_histogram, bins=bins_, color = 'r',alpha=0.1, label='BURST-send server')
    plt.legend(loc='best')
    plt.xscale('log')
    plt.title("BURST send histograms")
    plt.savefig(os.path.join(output,'BURST_send.pdf'), format='pdf')

    plt.figure()
    bins_=np.logspace(-3.0, 2.3, 50)
    plt.hist(client_BURST_receive_histogram, bins=bins_,color = 'g', alpha=0.5, label='BURST_receive client ')
    plt.hist(server_BURST_receive_histogram, bins=bins_, color = 'r',alpha=0.1, label='BURST_receive server')
    plt.legend(loc='best')
    plt.xscale('log')
    plt.title("BURST receive histograms")
    plt.savefig(os.path.join(output,'BURST_receive.pdf'), format='pdf')

    plt.figure()
    bins_=np.logspace(0.0, 3.0, 150)
    plt.hist(outgoing_burst_length, bins=bins_, color = 'g',alpha=0.5, label='outgoing ')
    plt.hist(incoming_burst_length, bins=bins_,color = 'r', alpha=0.2, label='incoming')
    plt.legend(loc='best')
    plt.title("Burst lengths")
    plt.xscale('log')
    plt.savefig(os.path.join(output,'burst_length.pdf'), format='pdf')

    plt.figure()
    plt.hist(number_of_burst_outgoing, 50,color = 'g', alpha=0.5, label='outgoing ')
    plt.hist(number_of_burst_incoming, 200,color = 'r', alpha=0.1, label='incoming')
    plt.legend(loc='best')
    plt.title("The number of bursts")
    plt.xlim(xmax = 200)

    plt.savefig(os.path.join(output,'number_of_burst.pdf'), format='pdf')
    print(("\nThe median of burst length in outgoing traffic: ",np.median(np.array(outgoing_burst_length))))
    print(("\nThe median of burst length in incoming traffic: ",np.median(np.array(incoming_burst_length))))
    print("\nDone!....\n")
    return client_GAP_send_histogram,client_BURST_send_histogram,client_BURST_receive_histogram, server_GAP_send_histogram,\
           server_BURST_send_histogram, server_BURST_receive_histogram, outgoing_burst_length,incoming_burst_length,\
number_of_burst_outgoing, number_of_burst_incoming


def parse_arguments():
    parser = argparse.ArgumentParser(description='Extract inter-arrival times to build histograms.',
                                     formatter_class=argparse.ArgumentDefaultsHelpFormatter)

    parser.add_argument('traces_path',
                        metavar='<traces path>',
                        help='Path to the directory with the traffic traces to be simulated.')
    parser.add_argument('--output', '-o',
                        type=str,
                        default='dump',
                        metavar='<oupput>',
                        help='Path to the directory where we save figures and dump data.')
    parser.add_argument('--neighbors', '-n',
                        default=2,
                        type=int,
                        metavar='<num neigbors>',
                        help='Number of neighbor around the current packet for computing bandwidth the window size will be 2*NEIGHBORS + 1.')
    parser.add_argument('--timestamp-column', '-t',
                        default=0,
                        type=int,
                        metavar='<timestamp column>',
                        help='Column index of timestamps in input traffic traces.')
    parser.add_argument('--packet-size-column', '-p',
                        default=1,
                        type=int,
                        metavar='<packet size column>',
                        help='Column index of packet sizes in input traffic traces.')
    parser.add_argument('--delimiter', '-d',
                        default='\t',
                        type=str,
                        metavar='<field delimiter>',
                        help='Field delimiter to split columns in input traffic traces.')

    return parser.parse_args()


def main():
    # parse arguments
    args = parse_arguments()

    client_GAP_send_histogram,client_BURST_send_histogram,client_BURST_receive_histogram, server_GAP_send_histogram,\
           server_BURST_send_histogram, server_BURST_receive_histogram, outgoing_burst_length,incoming_burst_length,\
number_of_burst_outgoing, number_of_burst_incoming = extract_times(**vars(args))

    hists = {
        'client_GAP_send_histogram':client_GAP_send_histogram,
        'client_BURST_send_histogram': client_BURST_send_histogram,
        'client_BURST_receive_histogram': client_BURST_receive_histogram,
        'server_GAP_send_histogram': server_GAP_send_histogram,
        'server_BURST_send_histogram': server_BURST_send_histogram,
        'server_BURST_receive_histogram': server_BURST_receive_histogram,
        'outgoing_burst_length': outgoing_burst_length,
        'incoming_burst_length': incoming_burst_length,
        'number_of_burst_outgoing': number_of_burst_outgoing,
        'number_of_burst_incoming': number_of_burst_incoming
    }

    dump_file(args.output, hists)


if __name__ == "__main__":
    try:
        sys.exit(main())
    except KeyboardInterrupt:
        sys.exit(1)

