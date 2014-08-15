#include "hackrf_tools.h"
#include "unistd.h"
#include <vector>


int main(int argc, char *argv[])
{
   Data_Publisher dp = Data_Publisher("A");
   std::vector<float> v;

   v.push_back(1.0);
   v.push_back(2.0);
   v.push_back(3.0);
   dp.Send(&v, x);

   for(int ii=0; ii<10; ii++) {
      v.clear();
      v.push_back(3.0 * ((double)ii/10.0));
      v.push_back(2.0 * ((double)ii/10.0));
      v.push_back(1.0 * ((double)ii/10.0));
      dp.Send(&v, y);
      sleep(1);
   }

   return 0;
}
