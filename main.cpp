#include<iostream>
#include<bits/stdc++.h>
#include "cache_header.h"
using namespace std;
#define ull unsigned long long

#define l1_blk_size    64
#define l1_assoc       8
#define l1_num_of_set  64

#define l2_assoc       16
#define l2_blk_size    64
#define l2_num_of_set  4096

#define num_proc 8

class directoryLine
{
    public:
        ull time;
        bool valid;
        bool dirtyBit;
        bool bitArray[8];
        ull owner;
};

unordered_map<ull,directoryLine> Directory;
unordered_map<ull,bool> Cold;
static ull Timer=0;
static ull Limit=0;
static ull L2_GET=0;
static ull L2_GETX=0;
static ull L2_ACK=0;
static ull SWB=0;
static ull STORE=0;
static ull UPGRADE_REQ=0;
void MESI(vector <Cache*> &cacheArrayL1,int proc_num,ull request,ull address,vector <Cache*> &cacheArrayL2)
{
    if(cacheArrayL1[proc_num]->hit_miss(address,Timer))
    {
        cacheArrayL1[proc_num]->hit++;
        //present in L1 cache
        if(request==0)
        {
            //don't do anything
        }
        else
        {
            cacheLine*ch=cacheArrayL1[proc_num]->getCacheLine(address);
            ull block_address=address/(l1_blk_size);
            if(ch->flag==SHARED)
            {
                directoryLine temp=Directory[block_address];
                if(temp.dirtyBit==1)//l1 to l1 cache transfer
                {
                    cout<<"Some thing bad has happened"<<endl;
                }
                else
                {
                    for(int i=0;i<num_proc;i++)
                    {
                        if(i!=proc_num&&temp.bitArray[i]==1)
                        {
                            temp.bitArray[i]=0;
                            cacheArrayL1[i]->Upgrade_Miss[address/l1_blk_size]=true;
                            cacheArrayL1[i]->Invalidate(address);
                            cacheArrayL1[i]->INV++;
                            cacheArrayL1[proc_num]->INV_ACK++;
                        }
                    }

                }
                temp.dirtyBit=1;
                temp.owner=proc_num;
                temp.bitArray[proc_num]=0;
                Directory[block_address]=temp;
                ch->flag=MODIFIED;
                cacheArrayL1[proc_num]->UPGRADE++;
                UPGRADE_REQ++;
            }
            if(ch->flag==EXCLUSIVE)
            {
                ch->flag=MODIFIED;
            }
        }
    }
    else
    {

        cacheArrayL1[proc_num]->miss++;
        ull block_address=address/(l1_blk_size);
        if(cacheArrayL1[proc_num]->Upgrade_Miss.find(block_address)!=cacheArrayL1[proc_num]->Upgrade_Miss.end()&&request==1)
        {
            cacheArrayL1[proc_num]->MISS++;
        }
        if(Directory.find(block_address)!=Directory.end()&&Directory[block_address].valid==1)//directory mai present hai
        {
            if(request==0)
            {
                directoryLine temp=Directory[block_address];
                if(temp.dirtyBit==1)//l1 to l1 cache transfer
                {

                    cacheLine *ch=cacheArrayL1[temp.owner]->getCacheLine(address);
                    ch->flag=SHARED;
                    temp.bitArray[temp.owner]=1;
                    cacheArrayL1[temp.owner]->GET++;
                    SWB++;
                    //yeha par SWB aur L1 to L1 transfer
                }
                else
                {
                    //yeha par L2 will provide the data
                     L2_GET++;
                     cacheArrayL2[(address/l2_blk_size)%(num_proc)]->hit_miss(address,Timer);////l2 will provide the data block
                     cacheArrayL2[(address/l2_blk_size)%(num_proc)]->hit++;
                }
                temp.dirtyBit=0;
                temp.bitArray[proc_num]=1;
                Directory[block_address]=temp;
            }
            else
            {
                directoryLine temp=Directory[block_address];
                if(temp.dirtyBit==1)//l1 to l1 cache transfer
                {
                    cacheArrayL1[temp.owner]->Invalidate(address);
                    temp.dirtyBit=0;
                    cacheArrayL1[temp.owner]->GETX++;
                    L2_ACK++;
                    //yeha par sirf cache to cache transfer
                }
                else
                {
                    for(int i=0;i<num_proc;i++)
                    {
                        if(temp.bitArray[i]==1)
                        {
                           cacheArrayL1[i]->INV++;
                           cacheArrayL1[proc_num]->INV_ACK++;
                            cacheArrayL1[i]->Invalidate(address);
                            cacheArrayL1[i]->Upgrade_Miss[address/l1_blk_size]=true;
                        }
                    }
                    //yeha par L2 will provide the data
                    L2_GET++;
                    cacheArrayL2[(address/l2_blk_size)%(num_proc)]->hit_miss(address,Timer);////l2 will provide the data block
                    cacheArrayL2[(address/l2_blk_size)%(num_proc)]->hit++;

                }
                temp.dirtyBit=1;
                temp.owner=proc_num;
                Directory[block_address]=temp;
            }
            //miss handler of l1
            ull Address_Evict=cacheArrayL1[proc_num]->Miss_Handler(address,Timer);
            if(Address_Evict!=-1)
            {
                directoryLine evicted=Directory[(Address_Evict/(l1_blk_size))];
                if(evicted.valid)
                {
                    if(evicted.dirtyBit==1)
                    {
                        evicted.valid=0;
                        evicted.dirtyBit=0;
                        STORE++;
                        //yeha par Store hoga
                    }
                    else
                    {
                        evicted.bitArray[proc_num]=0;
                        cacheArrayL1[proc_num]->Upgrade_Miss[Address_Evict/l1_blk_size]=true;
                        bool flag=false;
                        for(int i=0;i<8;i++)
                        {
                            if(evicted.bitArray[i]==1)
                            {
                                flag=true;
                                break;
                            }
                        }
                        if(!flag)
                        {
                            evicted.valid=0;//silent eviction
                        }
                    }
                }
                Directory[(Address_Evict/(l1_blk_size))]=evicted;
            }
            cacheLine * ch=cacheArrayL1[proc_num]->getCacheLine(address);
            if(request==0)
            {
                ch->flag=SHARED;
                cacheArrayL1[proc_num]->PUT++;
                //put generate hoga
            }
            else
            {
                ch->flag=MODIFIED;
                cacheArrayL1[proc_num]->PUTX++;
                //putx generate hoga
            }
        }
        else//directory mai nhi hai
        {
            ull bank_id=(address/l2_blk_size)%(num_proc);
            if(cacheArrayL2[bank_id]->hit_miss(address,Timer))
            {
                cacheArrayL2[bank_id]->hit++;
                directoryLine temp;
                temp.valid=1;
                temp.dirtyBit=1;
                temp.owner=proc_num;

                Directory[block_address]=temp;

                //miss handler of l1
                ull Address_Evict=cacheArrayL1[proc_num]->Miss_Handler(address,Timer);
                if(Address_Evict!=-1)
                {
                    directoryLine evicted=Directory[(Address_Evict/(l1_blk_size))];
                    if(evicted.valid)
                    {
                        if(evicted.dirtyBit==1)
                        {
                            evicted.valid=0;
                            evicted.dirtyBit=0;
                            STORE++;
                            //yeha par STORE hoga
                        }
                        else
                        {
                            evicted.bitArray[proc_num]=0;
                            cacheArrayL1[proc_num]->Upgrade_Miss[Address_Evict/l1_blk_size]=true;
                            bool flag=false;
                            for(int i=0;i<8;i++)
                            {
                                if(evicted.bitArray[i]==1)
                                {
                                    flag=true;
                                    break;
                                }
                            }
                            if(!flag)
                            {
                                evicted.valid=0;
                            }
                        }
                    }
                    Directory[(Address_Evict/(l1_blk_size))]=evicted;
                }
                cacheLine * ch=cacheArrayL1[proc_num]->getCacheLine(address);
                if(request==0)
                {
                    ch->flag=EXCLUSIVE;
                    (cacheArrayL1[proc_num]->PUTE)++;
                    L2_GET++;
                    //pute hoga
                }
                else
                {
                    ch->flag=MODIFIED;
                    (cacheArrayL1[proc_num]->PUTX)++;
                    L2_GETX++;
                    //putx hoga
                }


            }
            else//not present even in l2
            {
                cacheArrayL2[bank_id]->miss++;
                ull Add_Evi=cacheArrayL2[bank_id]->Miss_Handler(address,Timer);
                if(Add_Evi!=-1)//to maintain inclusion property
                {
                    ull Block_Add_Evi=Add_Evi/l1_blk_size;
                    directoryLine evicted = Directory[Block_Add_Evi];
                    if(evicted.valid)
                    {
                        if(evicted.dirtyBit==1)
                        {
                            cacheArrayL1[evicted.owner]->Invalidate(Add_Evi);
                            evicted.dirtyBit=0;
                            STORE++;
                            //yeha par STORE hoga
                        }
                        else
                        {
                            for(int i=0;i<num_proc;i++)
                            {
                                if(evicted.bitArray[i]==1)
                                {
                                    cacheArrayL1[i]->Invalidate(Add_Evi);
                                    cacheArrayL1[i]->Upgrade_Miss[Add_Evi/l1_blk_size]=true;
                                }
                            }
                        }
                    }
                    evicted.valid=0;
                    evicted.dirtyBit=0;
                    Directory[Block_Add_Evi]=evicted;
                }
                //update in directory
                directoryLine temp;
                temp.valid=1;
                temp.dirtyBit=1;
                temp.owner=proc_num;

                Directory[block_address]=temp;

                //miss handler of l1
                ull Address_Evict=cacheArrayL1[proc_num]->Miss_Handler(address,Timer);
                if(Address_Evict!=-1)
                {
                    directoryLine evicted=Directory[(Address_Evict/(l1_blk_size))];
                    if(evicted.valid)
                    {
                        if(evicted.dirtyBit==1)
                        {

                            evicted.valid=0;
                            evicted.dirtyBit=0;
                            STORE++;
                            //yeha par STORE hoga
                        }
                        else
                        {
                            evicted.bitArray[proc_num]=0;
                            cacheArrayL1[proc_num]->Upgrade_Miss[Address_Evict/l1_blk_size]=true;
                            bool flag=false;
                            for(int i=0;i<8;i++)
                            {
                                if(evicted.bitArray[i]==1)
                                {
                                    flag=true;
                                    break;
                                }
                            }
                            if(!flag)
                            {
                                evicted.valid=0;
                            }
                        }
                    }
                    Directory[(Address_Evict/(l1_blk_size))]=evicted;
                }
                //update of flag in L1 cache line
                cacheLine * ch=cacheArrayL1[proc_num]->getCacheLine(address);
                if(request==0)
                {
                    cacheArrayL1[proc_num]->PUTE++;
                    ch->flag=EXCLUSIVE;
                    L2_GET++;
                    //yeha par pute hoga
                }
                else
                {
                    ch->flag=MODIFIED;
                    cacheArrayL1[proc_num]->PUTX++;
                    L2_GETX++;
                    //yeha par putx hoga
                }
            }
        }
    }
}
int main(int argc,char**argv )
{
	if(argc<2)
	{
		cout<<"Please enter the trace file name"<<endl;
  		return 0;
	}
    FILE *f;
    f=fopen(argv[1],"rb");
   	vector <Cache*> cacheArrayL1;
	for(int i=0;i<num_proc;i++)
	{
		Cache*c=new Cache(l1_blk_size,l1_assoc,l1_num_of_set);
		cacheArrayL1.push_back(c);
	}

    ull* globalcount=new ull;
	ull* thread_id=new ull;
   	ull* request_type=new ull;
   	ull* addr=new ull;
   	//to take the trace file into each L1 trace_queue
    while(!feof(f))
    {
        fread(globalcount,sizeof(unsigned long long),1,f);
        fread(thread_id,sizeof(unsigned long long),1,f);
		fread(request_type,sizeof(unsigned long long),1,f);
		fread(addr,sizeof(unsigned long long),1,f);

		node*temp=new node(*addr,*request_type,*globalcount);
		cacheArrayL1[*thread_id]->trace_queue.push(temp);
		Limit++;
    }
    vector<Cache*> cacheArrayL2;
    for(int i=0;i<num_proc;i++)
    {
        Cache* d= new Cache(l2_blk_size,l2_assoc,l2_num_of_set);
        cacheArrayL2.push_back(d);
    }

    int sizeArray[8]={0};
	sizeArray[0]=cacheArrayL1[0]->trace_queue.size();
	sizeArray[1]=cacheArrayL1[1]->trace_queue.size();
	sizeArray[2]=cacheArrayL1[2]->trace_queue.size();
	sizeArray[3]=cacheArrayL1[3]->trace_queue.size();
	sizeArray[4]=cacheArrayL1[4]->trace_queue.size();
	sizeArray[5]=cacheArrayL1[5]->trace_queue.size();
	sizeArray[6]=cacheArrayL1[6]->trace_queue.size();
	sizeArray[7]=cacheArrayL1[7]->trace_queue.size();
    node*temp;
    while(Timer<=Limit)
    {
        for(int i=0;i<8;i++)
        {

                    if(sizeArray[i]!=0)
                    {
                        temp=cacheArrayL1[i]->trace_queue.front();
                        Cold[(temp->tag)/(l1_blk_size)]=true;
                        if(temp->value==Timer)
                        {
                            cacheArrayL1[i]->trace_queue.pop();
                            MESI(cacheArrayL1,i,temp->request_type,temp->tag,cacheArrayL2);
                            sizeArray[i]--;
                        }
                    }
        }
        Timer++;
    }
    cout<<"Stimulated Cycles "<<Limit<<endl;
    cout<<"L1 cache"<<endl;
    cout<<"ACCESS     "<<"HIT     "<<"MISS   "<<"UPGRADE MISS"<<endl;
    for(int i=0;i<num_proc;i++)
    {
        cout<<cacheArrayL1[i]->hit+cacheArrayL1[i]->miss+cacheArrayL1[i]->MISS<<" -- ";
        cout<<cacheArrayL1[i]->hit<<" -- ";
        cout<<cacheArrayL1[i]->miss<<" -- ";
        cout<<cacheArrayL1[i]->MISS<<endl;


    }
    cout<<"-------------------------------------------------------------"<<endl;
    cout<<"L2 cache"<<endl;
    ull L2_Miss=0,L2_Hit=0;
    for(int i=0;i<num_proc;i++)
    {
        L2_Miss+=cacheArrayL2[i]->miss;
        L2_Hit+=cacheArrayL2[i]->hit;
    }
    cout<<"Total Access "<<L2_Miss+L2_Hit<<endl;
    cout<<"Total Hit    "<<L2_Hit<<endl;
    cout<<"Total Miss   "<<L2_Miss<<endl;
    cout<<"-------------------------------------------------------------"<<endl;
    cout<<" Messages received by L1 caches"<<endl;
    cout<<endl;
    cout<<"PUT E Information"<<endl;
    for(int i=0;i<num_proc;i++)
    {
        cout<<"Proc "<<i<<" -- "<<cacheArrayL1[i]->PUTE<<endl;
    }
    cout<<"--------------------------------------------------------------"<<endl;
    cout<<"PUT X Information"<<endl;
    for(int i=0;i<num_proc;i++)
    {
        cout<<"Proc "<<i<<" -- "<<cacheArrayL1[i]->PUTX<<endl;
    }
    cout<<"--------------------------------------------------------------"<<endl;
    cout<<"PUT  Information"<<endl;
    for(int i=0;i<num_proc;i++)
    {
        cout<<"Proc "<<i<<" -- "<<cacheArrayL1[i]->PUT<<endl;
    }
    cout<<"--------------------------------------------------------------"<<endl;
    cout<<"GET  Information"<<endl;
    for(int i=0;i<num_proc;i++)
    {
        cout<<"Proc "<<i<<" -- "<<cacheArrayL1[i]->GET<<endl;
    }
    cout<<"--------------------------------------------------------------"<<endl;
    cout<<"GETX  Information"<<endl;
    for(int i=0;i<num_proc;i++)
    {
        cout<<"Proc "<<i<<" -- "<<cacheArrayL1[i]->GETX<<endl;
    }
    cout<<"--------------------------------------------------------------"<<endl;
    cout<<"UPGRADE_ACK  Information"<<endl;
    for(int i=0;i<num_proc;i++)
    {
        cout<<"Proc "<<i<<" -- "<<cacheArrayL1[i]->UPGRADE<<endl;
    }
    cout<<"--------------------------------------------------------------"<<endl;
    cout<<"INV  Information"<<endl;
    for(int i=0;i<num_proc;i++)
    {
        cout<<"Proc "<<i<<" -- "<<cacheArrayL1[i]->INV<<endl;
    }
    cout<<"--------------------------------------------------------------"<<endl;
    cout<<"INV_ACK Information"<<endl;
    for(int i=0;i<num_proc;i++)
    {
        cout<<"Proc "<<i<<" -- "<<cacheArrayL1[i]->INV_ACK<<endl;
    }
    cout<<"--------------------------------------------------------------"<<endl;
    cout<<"Cold Miss "<<Cold.size()<<endl;
    cout<<"--------------------------------------------------------------"<<endl;
    cout<<"--------------------------------------------------------------"<<endl;
    cout<<"Messages Received by  L2 Cache "<<endl;
    cout<<endl;
    cout<<"GET      "<<L2_GET<<endl;
    cout<<"GETX     "<<L2_GETX<<endl;
    cout<<"SWB      "<<SWB<<endl;
    cout<<"STORE    "<<STORE<<endl;
    cout<<"UPGRADE  "<<UPGRADE_REQ<<endl;
    cout<<"ACK      "<<L2_ACK<<endl;
    return 0;
}
