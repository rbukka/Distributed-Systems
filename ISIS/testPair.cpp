#include <map>
#include <string>
#include <stdio.h>
#include <iostream>
#include <cmath>

using namespace std;

class Key
{
  public: 
    Key(int seq, int pid)
    {
	     this->seq = seq;
	     this->pid = pid;
	  }
	  int seq;
	  int pid;
	  bool operator<(const Key& k) const
	  {
	      int s_cmp = (k.seq==this->seq);
	      if(s_cmp == 1)
		    {
			    return this->pid < k.pid;
			  }
			  return (k.seq>this->seq);
		}
};

int main()
{
	int y = 9;
	double x = 9.0;
	double d = log(y);
	double t = log(x);

	printf("%lf %lf\n",d,t);
	/*
	double d = double(10.5);
	printf("%lf\n",d);


	Key p1 (2, 20);
	Key p2 (1, 3);
	Key p3 (0,1000);
	Key p4 (-1,0);
	Key p5 (1,6);
	Key p6 (0,111);
	Key p7 (-2,1121);
	
	std::map<Key,std::string> mapa;
	
	mapa[p1] = "Manzana";
	mapa[p2] = "Arandano";
	mapa[p3] = "Orange";
	mapa[p4] = "Apple";
	mapa[p5] = "Ravi";
	mapa[p6] = "kks";
	mapa[p7] = "Asd";
	
	for(map<Key,string>:: iterator it=mapa.begin();it!=mapa.end();++it)
	{
		std::cout	<< it->first.seq << " " << it->first.pid << " ==> " << it->second << '\n' ;
	}
	return 0;
	*/
}
