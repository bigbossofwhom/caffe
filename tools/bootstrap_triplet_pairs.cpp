#include <gflags/gflags.h>
#include <glog/logging.h>
#include <sys/stat.h>

#include <algorithm>
#include <fstream>  // NOLINT(readability/streams)
#include <string>
#include <utility>
#include <vector>
#include <stdlib.h> 
#include <time.h> 

#include "caffe/proto/caffe.pb.h"
#include "caffe/util/io.hpp"
#include "caffe/util/rng.hpp"

using std::pair;
using std::string;

DEFINE_string(alpha, "0.2", "The threshold");
DEFINE_int32(num_neg_sample, 1000, "The number of negative samples");



int _MAX_PATH = 10000;

// get size of features
static int get_file_size(const char *filename)
{
  int fsize;
  FILE *fp;

  fp = fopen(filename, "rb");
  if(fp == NULL)
    return 0;
  fseek(fp, 0, SEEK_END);
  fsize = ftell(fp);
  fclose(fp);

  return fsize;
}

// read features
static unsigned char * read_file_buffer(const char *filename, int *size)
{
  int fsize;
  FILE *fp;
  unsigned char *pnchBuf = NULL;

  *size = 0;

  fsize = get_file_size(filename);
  if(fsize == 0)
    return NULL;

  if((fp = fopen(filename, "rb")) != NULL )
  {
    pnchBuf = (unsigned char *) malloc(fsize * sizeof(unsigned char));
    fread(pnchBuf, fsize, 1, fp);
    fclose(fp);
  }

  *size = fsize;

  return pnchBuf;
}

// L2 distance compute
static float compute_L2_distance(float* feat1, float* feat2, int feat_len)
{
  float distance = 0;
  for (int i = 0; i < feat_len; ++i )
  {
    distance += (feat1[i] - feat2[i])*(feat1[i] - feat2[i]);
  }
  return distance;
}


int main(int argc, char** argv)
{
  ::google::InitGoogleLogging(argv[0]);
  ::google::SetLogDestination(0, argv[1]);

#ifndef GFLAGS_GFLAGS_H_
  namespace gflags = google;
#endif

  // bootstrap_triplet_pairs log_path feature_dir feature_list save_pairs_data_list
  const int num_required_args = 3;

  std::ifstream infile(argv[3]);
  std::vector<std::pair<string, int> > lines;
  string filename;
  int label;

  // read feature list
  while (infile >> filename >> label)
  {
    lines.push_back(std::make_pair(filename, label));
  }
  LOG(ERROR) << "A total of " << lines.size() << " features.";
  infile.close();

  // load features
  string root_folder(argv[2]);
  char file_name[_MAX_PATH];
  string str_tmp;
  int i, file_size, feat_len;
  float **features;

  features = (float **) malloc(lines.size() * sizeof(float *));
  LOG(ERROR) << "Loading features...";
  for(i=0; i<lines.size(); i++)
  {
    str_tmp = root_folder + lines[i].first;
    strcpy(file_name, str_tmp.c_str());
    features[i] = (float *) read_file_buffer(file_name, &file_size);

    if(i % 10000 == 0)
    {
      LOG(ERROR) << "Loaded " << i << " features.";
    }
  }
  feat_len = file_size / sizeof(float);
  LOG(ERROR) << "feature length: " << feat_len;

  float alpha = atof(FLAGS_alpha.c_str());
  int num_neg_sample = FLAGS_num_neg_sample;

  LOG(ERROR) << "alpha = " << alpha << ", num_neg = " << num_neg_sample;


  // anchor
  std::ofstream outfile(argv[4]);
  int cnt = 0;

  for (int idx_anchor = 0; idx_anchor < lines.size(); ++idx_anchor)
  {
    float* feat_anchor = features[idx_anchor];
    int label_anchor = lines[idx_anchor].second;
    for (int idx_pos = 0; idx_pos < lines.size(); ++idx_pos)
    {
      int label_pos = lines[idx_pos].second;
      if((idx_anchor == idx_pos) || (label_anchor != label_pos))
        continue;
      float* feat_pos = features[idx_pos];
      float pos_scores = compute_L2_distance(feat_anchor, feat_pos, feat_len);
      

      srand((unsigned)time(NULL));
      int count = 0;
      while(count < num_neg_sample)
      {
        int idx_neg = rand() % lines.size();
        int label_neg = lines[idx_neg].second;
        if(label_anchor == label_neg)
          continue;
        float* feat_neg = features[idx_neg];
        float neg_scores = compute_L2_distance(feat_anchor, feat_neg, feat_len);

        if(cnt % 10000)
        {
          LOG(ERROR) << "anchor: " << idx_anchor << " pos: " 
            << idx_pos << " neg: " << idx_neg << " pos_scores: " 
            << pos_scores << " neg_scores: " << neg_scores;
        }
        

        if (pos_scores < neg_scores && pos_scores + alpha > neg_scores)
        {
          outfile << idx_anchor << " " << idx_pos << " " << idx_neg << "\n";
        }
        count++;
        cnt++;
      }
    }
  }

  outfile.close();

}
