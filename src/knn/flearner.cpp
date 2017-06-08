/*
 * This code applies the k-NN classifier on the features files extracted with
 * `fextractor.py`.
 * 
 * This code is based on the original implementation by Wang et al. and
 * modified to fit our purposes.
 *
 */
#include <iostream>
#include <fstream>
#include <cmath>
#include <string>
#include <string.h>
#include <sstream>
#include <time.h>
#include <stdlib.h>
#include <algorithm>
using namespace std;

//Data parameters
int FEAT_NUM = 735;//3736; //number of features

int SITE_NUM = 10; //number of monitored sites
int INST_NUM = 5; //number of instances per site for distance learning
int TEST_NUM = 5; //number of instances per site for kNN training/testing

int OPENTEST_NUM = 0; //number of open instances for kNN training/testing
int NEIGHBOUR_NUM = 1; //number of neighbors for kNN

const int RECOPOINTS_NUM = 5; //number of neighbors for distance learning

//Algorithmic Parameters
float POWER = 0.1; //not used in this code

//Only in old recommendation algorithm
const int RECOLIST_NUM = 10;
const int RECO_NUM = 1;


bool inarray(int ele, int* array, int len) {
	for (int i = 0; i < len; i++) {
		if (array[i] == ele)
			return 1;
	}
	return 0;
}

void alg_init_weight(float** feat, float* weight) {
	for (int i = 0; i < FEAT_NUM; i++) {
		weight[i] = (rand() % 100) / 100.0 + 0.5;
	}
}

float dist(float* feat1, float* feat2, float* weight, float power) {
	float toret = 0;
	for (int i = 0; i < FEAT_NUM; i++) {
		if (feat1[i] != -1 and feat2[i] != -1) {
			toret += weight[i] * abs(feat1[i] - feat2[i]);
		}
	}
	return toret;
}

void alg_recommend2(float** feat, float* weight, int start, int end) {

	float* distlist = new float[SITE_NUM * INST_NUM];
	int* recogoodlist = new int[RECOPOINTS_NUM];
	int* recobadlist = new int[RECOPOINTS_NUM];

	for (int i = start; i < end; i++) {
		int cur_site = i/INST_NUM;
		int cur_inst = i % INST_NUM;

		float pointbadness = 0;
		float maxgooddist = 0;

		for (int k = 0; k < SITE_NUM*INST_NUM; k++) {
			distlist[k] = dist(feat[i], feat[k], weight, POWER);
		}
		float max = *max_element(distlist, distlist+SITE_NUM*INST_NUM);
		distlist[i] = max;
		for (int k = 0; k < RECOPOINTS_NUM; k++) {
			int ind = min_element(distlist+cur_site*INST_NUM, distlist+(cur_site+1)*INST_NUM) - distlist;
			if (distlist[ind] > maxgooddist) maxgooddist = distlist[ind];
			distlist[ind] = max;
			recogoodlist[k] = ind;
		}
		for (int k = 0; k < INST_NUM; k++) {
			distlist[cur_site*INST_NUM+k] = max;
		}
		for (int k = 0; k < RECOPOINTS_NUM; k++) {
			int ind = min_element(distlist, distlist+ SITE_NUM * INST_NUM) - distlist;
			if (distlist[ind] <= maxgooddist) pointbadness += 1;
			distlist[ind] = max;
			recobadlist[k] = ind;
		}

		pointbadness /= float(RECOPOINTS_NUM);
		pointbadness += 0.2;

		float* featdist = new float[FEAT_NUM];
		for (int f = 0; f < FEAT_NUM; f++) {
			featdist[f] = 0;
		}
		int* badlist = new int[FEAT_NUM];
		int minbadlist = 0;
		int countbadlist = 0;
		for (int f = 0; f < FEAT_NUM; f++) {
			if (weight[f] == 0) badlist[f] == 0;
			else {
			float maxgood = 0;
			int countbad = 0;
			for (int k = 0; k < RECOPOINTS_NUM; k++) {
				float n = abs(feat[i][f] - feat[recogoodlist[k]][f]);
				if (feat[i][f] == -1 or feat[recobadlist[k]][f] == -1) 
					n = 0;
				if (n >= maxgood) maxgood = n;
			}
			for (int k = 0; k < RECOPOINTS_NUM; k++) {
				float n = abs(feat[i][f] - feat[recobadlist[k]][f]);
				if (feat[i][f] == -1 or feat[recobadlist[k]][f] == -1) 
					n = 0;
				featdist[f] += n;
				if (n <= maxgood) countbad += 1;
			}
			badlist[f] = countbad;
			if (countbad < minbadlist) minbadlist = countbad;	
			}
		}

		for (int f = 0; f < FEAT_NUM; f++) {
			if (badlist[f] != minbadlist) countbadlist += 1;
		}
		int* w0id = new int[countbadlist];
		float* change = new float[countbadlist];

		int temp = 0;
		float C1 = 0;
		float C2 = 0;
		for (int f = 0; f < FEAT_NUM; f++) {
			if (badlist[f] != minbadlist) {
				w0id[temp] = f;
				change[temp] = weight[f] * 0.01 * badlist[f]/float(RECOPOINTS_NUM) * pointbadness;
				C1 += change[temp] * featdist[f];
				C2 += change[temp];
				weight[f] -= change[temp];
				temp += 1;
			}
		}

		float totalfd = 0;
		for (int f = 0; f < FEAT_NUM; f++) {
			if (badlist[f] == minbadlist and weight[f] > 0) {
				totalfd += featdist[f];
			}
		}

		for (int f = 0; f < FEAT_NUM; f++) {
			if (badlist[f] == minbadlist and weight[f] > 0) {
				weight[f] += C1/(totalfd);
			}
		}
		delete[] featdist;
		delete[] w0id;
		delete[] change;
		delete[] badlist;
	}


	for (int j = 0; j < FEAT_NUM; j++) {
		if (weight[j] > 0)
			weight[j] *= (0.9 + (rand() % 100) / 500.0);
	}
	delete[] distlist;
	delete[] recobadlist;
	delete[] recogoodlist;
}

//no longer used
void alg_recommend(float** feat, float* weight, float** reco) {

	float* distlist = new float[SITE_NUM * INST_NUM];
	int* recogoodlist = new int[RECOLIST_NUM];
	int* recobadlist = new int[RECOLIST_NUM];

	for (int i = 0; i < SITE_NUM * INST_NUM; i++) {
		int cur_site = i/INST_NUM;
		int cur_inst = i % INST_NUM;

		for (int k = 0; k < SITE_NUM*INST_NUM; k++) {
			distlist[k] = dist(feat[i], feat[k], weight, POWER);
		}
		float max = *max_element(distlist, distlist+SITE_NUM*INST_NUM);
		distlist[i] = max;
		for (int k = 0; k < RECOLIST_NUM; k++) {
			int ind = min_element(distlist+cur_site*INST_NUM, distlist+(cur_site+1)*INST_NUM) - distlist;
			distlist[ind] = max;
			recogoodlist[k] = ind;
		}
		for (int k = 0; k < INST_NUM; k++) {
			distlist[cur_site*INST_NUM+k] = max;
		}
		for (int k = 0; k < RECOLIST_NUM; k++) {
			int ind = min_element(distlist, distlist+ SITE_NUM * INST_NUM) - distlist;
			distlist[ind] = max;
			recobadlist[k] = ind;
		}

		for (int f = 0; f < FEAT_NUM; f++) {
			reco[i][f] = 0;
		}

		if (i == 0) {
			float gooddist = 0;
			float baddist = 0;
			printf("Current point: %d\n", i);
			printf("Bad points:\n");
			for (int k = 0; k < RECOPOINTS_NUM; k++) {
				printf("%d, %f\n", recobadlist[k], dist(feat[i], feat[recobadlist[k]], weight, POWER));	
				baddist += dist(feat[i], feat[recobadlist[k]], weight, POWER);
			}

			printf("Good points:\n");
			for (int k = 0; k < RECOPOINTS_NUM; k++) {
				printf("%d, %f\n", recogoodlist[k], dist(feat[i], feat[recogoodlist[k]], weight, POWER));
				gooddist += dist(feat[i], feat[recogoodlist[k]], weight, POWER);
			}

			printf("Total bad distance: %f\n", baddist);
			printf("Total good distance: %f\n", gooddist);

			float* tempweight = new float[FEAT_NUM];

			for (int f = 0; f < FEAT_NUM; f++) {
				tempweight[f] = 0;
			}
			for (int k = 0; k < RECOPOINTS_NUM; k++) {
				int ind1 = recobadlist[k];
				int ind2 = recogoodlist[k];
				for (int f = 0; f < FEAT_NUM; f++) {
					tempweight[f] += abs(feat[i][f] - feat[ind1][f]); 
					tempweight[f] -= abs(feat[i][f] - feat[ind2][f]); 
				}
			}
			
			for (int f = 0; f < 25; f++) {
				printf("Weight %d: Value %f, Change %f\n", f, weight[f], tempweight[f]);
			}

			float sumweight = 0;

			for (int f = 0; f < FEAT_NUM; f++) {
				if (tempweight[f] < 0) tempweight[f] = 0;
				sumweight += abs(tempweight[f]);
			}

			for (int f = 0; f < FEAT_NUM; f++) {
				tempweight[f] /= sumweight;
				tempweight[f] += weight[f];
			}

			baddist = 0;
			gooddist = 0;
			printf("Bad points:\n");
			for (int k = 0; k < RECOPOINTS_NUM; k++) {
				printf("%d, %f\n", recobadlist[k], dist(feat[i], feat[recobadlist[k]], tempweight, POWER));	
				baddist += dist(feat[i], feat[recobadlist[k]], tempweight, POWER);
			}

			printf("Good points:\n");
			for (int k = 0; k < RECOPOINTS_NUM; k++) {
				printf("%d, %f\n", recogoodlist[k], dist(feat[i], feat[recogoodlist[k]], tempweight, POWER));
				gooddist += dist(feat[i], feat[recogoodlist[k]], tempweight, POWER);
			}

			printf("Total bad distance: %f\n", baddist);
			printf("Total good distance: %f\n", gooddist);
			delete[] tempweight;
		}

		for (int k = 0; k < RECOPOINTS_NUM; k++) {
			int ind1 = recobadlist[k];
			int ind2 = recogoodlist[k];
			for (int f = 0; f < FEAT_NUM; f++) {
				reco[i][f] += abs(feat[i][f] - feat[ind1][f]); 
				reco[i][f] -= abs(feat[i][f] - feat[ind2][f]); 
			}
		}
	}
	delete[] distlist;
	delete[] recobadlist;
	delete[] recogoodlist;
	
}

//no longer used
void alg_mod_weight(float* weight, float** reco) {
	float votesum = 0;
	float* sumreco = new float[FEAT_NUM];
	for (int i = 0; i < FEAT_NUM; i++){
		sumreco[i] = 0;
	}
	for (int i = 0; i < SITE_NUM*INST_NUM*RECO_NUM; i++) {
		for (int j = 0; j < FEAT_NUM; j++) {
			sumreco[j] += reco[i][j];
		}
	}

	for (int i = 0; i < FEAT_NUM; i++) {
		sumreco[i] /= SITE_NUM*INST_NUM*RECO_NUM;
	}
	for (int j = 0; j < FEAT_NUM; j++) {
		weight[j] += sumreco[j] * 40;
		if (weight[j] < 0) weight[j] = 0;
	}
	for (int j = 0; j < FEAT_NUM; j++) {
		if (weight[j] > 0)
			weight[j] *= (0.5 + (rand() % 100) / 100.0);
	}

	float sum = 0;
	for (int j = 0; j < FEAT_NUM; j++) {
		if (abs(weight[j]) > sum) {
		sum += abs(weight[j]);
		}
	}
	for (int j = 0; j < FEAT_NUM; j++) {
		weight[j] = weight[j]/sum;
	}
	delete[] sumreco;
}


//no longer used
void alg_change_weight(float* featdist, float* weight, int* w0id, float* change, int changenum) {
	//Changes weight, keeping sum_k featdist[k]*weight[k] constant, weights between 0 and 1
	//change assumed to be legal, weights assumed to be legal. change aways positive
	//change legal if w0 >= average. 
	//featdist[weightnum] can't be the largest!
	
	float C1 = 0;
	float C2 = 0;

	for (int i = 0; i < changenum; i++) {
		C1 += featdist[w0id[i]] * change[i];
		C2 += change[i];
		weight[w0id[i]] -= change[i];
	}

	int w1choices = 0;
	for (int i = 0; i < FEAT_NUM; i++) {
		if (featdist[i] > C1/C2 and !inarray(i, w0id, changenum)) {
			w1choices += 1;
		}
	}
	int w1rand = (rand() % w1choices) + 1;
	int w1id = 0;

	int count = 0;
	float w1 = 0;
	for (int i = 0; i < FEAT_NUM; i++) {
		if (featdist[i] > C1/C2 and !inarray(i, w0id, changenum)) count += 1;
		if (w1rand == count) {
			w1 = featdist[i];
			w1id = i;
			break;
		}
	}

	float othersum = 0;
	int othernum = 0;
	for (int i = 0; i < FEAT_NUM; i++) {
		if (!inarray(i, w0id, changenum) and featdist[i] != 0) {
			othersum += featdist[i];
			othernum += 1;
		}
	}
	othersum /= othernum;

	//solution:
	float x1 = (C1 - othersum * C2)/(w1-othersum);
	weight[w1id] += x1;

	float sum = 0;
	for (int i = 0; i < FEAT_NUM; i++) {
		if (!inarray(i, w0id, changenum) and featdist[i] != 0) {
			weight[i] += (C2 - x1)/othernum;
		}
	}
}

void accuracy(float** closedfeat, float* weight, float** openfeat, float & tp, float & tn, float & fp, float & fn) {

	tp = 0;
	tn = 0;
	fp = 0;
	fn = 0;

	float** feat = new float*[SITE_NUM*TEST_NUM + OPENTEST_NUM];

	for (int i = 0; i < SITE_NUM*TEST_NUM; i++) {
		feat[i] = closedfeat[i];
	}
	for (int i = 0; i < OPENTEST_NUM; i++) {
		feat[i + SITE_NUM * TEST_NUM] = openfeat[i];
	}

	float* distlist = new float[SITE_NUM * TEST_NUM + OPENTEST_NUM];
	int* classlist = new int[SITE_NUM + 1];

	float* opendistlist = new float[OPENTEST_NUM];

	for (int is = 0; is < SITE_NUM*TEST_NUM + OPENTEST_NUM; is++) {
		for (int i = 0; i < SITE_NUM+1; i++) {
			classlist[i] = 0;
		}
		int maxclass = 0;
		for (int at = 0; at < SITE_NUM * TEST_NUM + OPENTEST_NUM; at++) {
			distlist[at] = dist(feat[is], feat[at], weight, POWER);
		}
		float max = *max_element(distlist, distlist+SITE_NUM*TEST_NUM+OPENTEST_NUM);
		distlist[is] = max;
		for (int i = 0; i < NEIGHBOUR_NUM; i++) {
			int ind = find(distlist, distlist + SITE_NUM*TEST_NUM+OPENTEST_NUM, *min_element(distlist, distlist+SITE_NUM*TEST_NUM+OPENTEST_NUM)) - distlist;
			int classind = 0;
			if (ind < SITE_NUM * TEST_NUM) {
				classind = ind/TEST_NUM;
			}
			else {
				classind = SITE_NUM;
			}
			classlist[classind] += 1;
			if (classlist[classind] > maxclass) {
				maxclass = classlist[classind];
			}
			distlist[ind] = max;
		}

		int trueclass = is/TEST_NUM;
		if (trueclass > SITE_NUM) trueclass = SITE_NUM;

		int countclass = 0;
		int hascorrect = 0;

		int hasconsensus = 0;
		for (int i = 0; i < SITE_NUM+1; i++) {
			if (classlist[i] == NEIGHBOUR_NUM) {
				hasconsensus = 1;
			}
		}
		if (hasconsensus == 0) {
			for (int i = 0; i < SITE_NUM; i++) {
				classlist[i] = 0;
			}
			classlist[SITE_NUM] = 1;
			maxclass = 1;
		}

		for (int i = 0; i < SITE_NUM+1; i++) {
			if (classlist[i] == maxclass) {
				countclass += 1;
				if (i == trueclass) {
					hascorrect = 1;
				}
			}
		}

		float thisacc = 0;
		if (hascorrect == 1) {
			thisacc = 1.0/countclass;
			if (trueclass == SITE_NUM) {
				tn += thisacc;
			}
			else { 
				tp += thisacc;
			}
		}else{
			thisacc = 1.0/countclass;
			if (trueclass == SITE_NUM) {
				fn += thisacc;
			}
			else { 
				fp += thisacc;
			}
		}
	}
	delete[] distlist;
	delete[] classlist;
	delete[] opendistlist;
	delete[] feat;
	
	tp /= SITE_NUM*TEST_NUM;
	fp /= SITE_NUM*TEST_NUM;
	if (OPENTEST_NUM > 0){
		tn /= OPENTEST_NUM;
		fn /= OPENTEST_NUM;
	}else{
		 tn = 0;
		fn = 0;
	}
}


int main(int argc, char** argv) {

	int OPENTEST_list [12] = {0, 10, 50, 100, 200, 300, 500, 1000, 1500, 2000, 3000, 5000};
	int NEIGHBOUR_list [12] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 12, 15};

	if(argc == 4){
		SITE_NUM = atoi(argv[1]); //number of monitored sites
		INST_NUM = atoi(argv[2]); //number of instances per site for distance learning
		TEST_NUM = atoi(argv[3]); //number of instances per site for kNN training/testing
	}

	//printf("Classes: %d training %d testing %d\n\n", SITE_NUM,INST_NUM, TEST_NUM);
	srand(time(NULL));
	
	OPENTEST_NUM = 0;
	//NEIGHBOUR_NUM = 1;

	float** feat = new float*[SITE_NUM*INST_NUM];
	float** testfeat = new float*[SITE_NUM*TEST_NUM];
	float** opentestfeat = new float*[OPENTEST_NUM];

	for (int i = 0; i < SITE_NUM*INST_NUM; i++) {
		feat[i] = new float[FEAT_NUM];
	}
	for (int i = 0; i < SITE_NUM*TEST_NUM; i++) {
		testfeat[i] = new float[FEAT_NUM];
	}
	for (int i = 0; i < OPENTEST_NUM; i++) {
		opentestfeat[i] = new float[FEAT_NUM];
	}

	for (int cur_site = 0; cur_site < SITE_NUM; cur_site++) {
		int real_inst = 0;
		for (int cur_inst = 0; cur_inst < INST_NUM; cur_inst++) {
			int gotfile = 0;
			ifstream fread;
			while (gotfile == 0) {
				ostringstream freadnamestream;
				freadnamestream << "batch/" << cur_site << "-" << real_inst << "f";
				string freadname = freadnamestream.str();
				fread.open(freadname.c_str());
				if (fread.is_open()) {
					gotfile = 1;
					//printf("Read weight learn trace: %s\n", freadname.c_str());
				}
				real_inst++;
			}
			string str = "";
			getline(fread, str);
			fread.close();

			string tempstr = "";
			int feat_count = 0;
			for (int i = 0; i < str.length(); i++) {
				if (str[i] == ' ') {
					if (tempstr.c_str()[1] == 'X') {
						feat[cur_site * INST_NUM + cur_inst][feat_count] = -1;
					}
					else {
						feat[cur_site * INST_NUM + cur_inst][feat_count] = atof(tempstr.c_str());
					}	
					feat_count += 1;
					tempstr = "";
				}
				else {
					tempstr += str[i];
				}
			}
			
		}
		for (int cur_inst = 0; cur_inst < TEST_NUM; cur_inst++) {
			int gotfile = 0;
			ifstream fread;
			string freadname;
			while (gotfile == 0) {
				ostringstream freadnamestream;
				freadnamestream << "batch/" << cur_site << "-" << real_inst << "f";
				freadname = freadnamestream.str();
				fread.open(freadname.c_str());
				if (fread.is_open()) gotfile = 1;
				real_inst++;
			}
			string str = "";
			getline(fread, str);
			fread.close();

			string tempstr = "";
			int feat_count = 0;
			for (int i = 0; i < str.length(); i++) {
				if (str[i] == ' ') {
					if (tempstr.c_str()[1] == 'X') {
						testfeat[cur_site * TEST_NUM + cur_inst][feat_count] = -1;
					}
					else {
						testfeat[cur_site * TEST_NUM + cur_inst][feat_count] = atof(tempstr.c_str());
					}	
					feat_count += 1;
					tempstr = "";
				}
				else {
					tempstr += str[i];
				}
			}
		}
	}

	for (int cur_site = 0; cur_site < OPENTEST_NUM; cur_site++) {
			int gotfile = 0;
			ifstream fread;
			string freadname;
			while (gotfile == 0) {
				ostringstream freadnamestream;
				freadnamestream << "batch/" << cur_site << "f";
				freadname = freadnamestream.str();
				fread.open(freadname.c_str());
				if (fread.is_open()) gotfile = 1;
			}
			string str = "";
			getline(fread, str);
			fread.close();

			string tempstr = "";
			int feat_count = 0;
			for (int i = 0; i < str.length(); i++) {
				if (str[i] == ' ') {
					if (tempstr.c_str()[1] == 'X') {
						opentestfeat[cur_site][feat_count] = -1;
					}
					else {
						opentestfeat[cur_site][feat_count] = atof(tempstr.c_str());
					}	
					feat_count += 1;
					tempstr = "";
				}
				else {
					tempstr += str[i];
				}
			}
	}



	float * weight = new float[FEAT_NUM];
	float * value = new float[FEAT_NUM];

	int TRIAL_NUM = 1;
	int SUBROUND_NUM = 1;
	float maxacc = 0;
	float t_tn,t_fp, t_fn;
	alg_init_weight(feat, weight);

	float * prevweight = new float[FEAT_NUM];
	for (int i = 0; i < FEAT_NUM; i++) {
		prevweight[i] = weight[i];
	}

	clock_t t1, t2;
	alg_init_weight(feat, weight);
	for (int trial = 0; trial < TRIAL_NUM; trial++) {
		for (int subround = 0; subround < SUBROUND_NUM; subround++) {
			int start = (SITE_NUM * INST_NUM)/SUBROUND_NUM * subround;
			int end = (SITE_NUM * INST_NUM)/SUBROUND_NUM * (subround+1);
			alg_recommend2(feat, weight, start, end);
			float tp, tn,fp, fn;
			accuracy(testfeat, weight, opentestfeat, tp, tn,fp, fn);
			if (tp > maxacc){ 
				maxacc = tp;
				t_fp = fp;
				t_tn = tn;
				t_fn = fn;

			}
		}
	}


	FILE * myfile;
	myfile = fopen("accuracy", "w");
	fprintf(myfile, "%f",maxacc);
	fclose(myfile);

	for (int i = 0; i < SITE_NUM * INST_NUM; i++) {
		delete[] feat[i];
	}
	delete[] feat;
	for (int i = 0; i < SITE_NUM * TEST_NUM; i++) {
		delete[] testfeat[i];
	}
	delete[] testfeat;
	for (int i = 0; i < OPENTEST_NUM; i++) {
		delete[] opentestfeat[i];
	}
	delete[] opentestfeat;

	delete[] prevweight;
	delete[] weight;
	delete[] value;
	return 0;
}
