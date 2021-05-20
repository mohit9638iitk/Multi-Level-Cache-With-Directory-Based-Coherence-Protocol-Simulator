#include<iostream>
#include<bits/stdc++.h>

#define ull unsigned long long
#define uchar unsigned char

using namespace std;

enum{
    INVALID=0,
    SHARED,
    MODIFIED,
    EXCLUSIVE,
    PENDING
};
class node
{
    public:
        ull tag;
        ull request_type;
        ull value;
        node*next;
        node(ull a,ull r,ull b)
        {
            tag=a;
            request_type=r;
            value=b;
            next=NULL;
        }
};

class cacheLine
{
    public:
        ull flag;
        ull tag;
        ull time;
};

class Cache
{
    public:
        ull blk_size,num_of_set,associativity;
        queue<node*>trace_queue;
        queue<node*>request_queue;
        unordered_map<ull,bool>Upgrade_Miss;
        cacheLine** cache;
        ull hit,miss;
        ull PUTE;
        ull PUTX;
        ull PUT;
        ull GET;
        ull GETX;
        ull UPGRADE;
        ull INV_ACK;
        ull INV;
        ull MISS=0;
        Cache(ull b,ull a,ull s)
        {
            blk_size=b;
            num_of_set=s;
            associativity=a;

            cache=new cacheLine*[num_of_set*sizeof(cacheLine*)];
            for(int i=0;i<num_of_set;i++)
            {
                cache[i]=new cacheLine[associativity*sizeof(cacheLine)];
                for(int j=0;j<associativity;j++)
                {
                    cache[i][j].flag=INVALID;
                }
            }
            hit=0;
            miss=0;
            PUTE=0;
            PUTX=0;
            PUT=0;
            GET=0;
            GETX=0;
            UPGRADE=0;
            INV_ACK=0;
            INV=0;
            MISS=0;
        }


        bool hit_miss(ull addr,ull Time)
        {
            ull index=(addr/blk_size)%(num_of_set);
            ull tag=(addr/(blk_size*num_of_set));

            for(int i=0;i<associativity;i++)
            {
                if(cache[index][i].flag && cache[index][i].tag == tag)
                {
                    cache[index][i].time = Time;
                    return true;
                }
            }
            return false;
        }
        int Invalidate(ull Addr)
        {
            int Idx = (Addr / (blk_size))%(num_of_set);
            ull Tag = (Addr / (blk_size*num_of_set));

            for(int i=0;i<associativity;i++)
            {
                if(cache[Idx][i].flag && cache[Idx][i].tag == Tag)
                {
                    cache[Idx][i].flag = 0;
                    return 1;
                }
            }
            return 0;
        }
        cacheLine* getCacheLine(ull Addr)
        {
            int Idx = (Addr / (blk_size))%(num_of_set);
            ull Tag = (Addr / (blk_size*num_of_set));

            for(int i=0;i<associativity;i++)
            {
                if(cache[Idx][i].flag && cache[Idx][i].tag == Tag)
                {
                    return &(cache[Idx][i]);
                }
            }
            return NULL;


        }
        int LRU_Block(cacheLine * Set)
        {
            int min = 0;
            for(int i=0;i<associativity;i++)
            {
                if(Set[i].flag== 0)
                {
                    min = i;
                    break;
                }
                else if(Set[i].time < Set[min].time)
                {
                    min = i;
                }

            }
            return min;
        }

        ull Miss_Handler(ull Addr, ull Time)
        {
            int Idx = (Addr / (blk_size))%(num_of_set);
            ull Tag = (Addr / (blk_size*num_of_set));
            ull Add_Evi;

            int R_Way = LRU_Block(cache[Idx]);
            ull Tag_Evi;
            if(cache[Idx][R_Way].flag!=0)
            {
                Tag_Evi = cache[Idx][R_Way].tag;
                Add_Evi = (Tag_Evi*num_of_set + Idx)*blk_size;
            }
            else
            {
                Add_Evi = -1;
            }

            cache[Idx][R_Way].tag = Tag;
            cache[Idx][R_Way].flag = 1;
            cache[Idx][R_Way].time = Time;

            return Add_Evi;
        }

};

