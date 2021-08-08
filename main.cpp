#include <iostream>
#include <stdlib.h>
#include<stdio.h>

using namespace std;

/* run this program using the console pauser or add your own getch, system("pause") or input loop */

struct Node
{
	int data;
	struct Node *next;	//self-referential pointer
}*first; //global variable

void create(int A[], int n)
{
	int i=0;
	struct Node *t,*last;
	first = (struct Node*) malloc(sizeof(struct Node));
	first->data=A[0];
	first->next=NULL;
	last=first;
	
	for(i=1;i<n;i++)
	{
		t=(struct Node*)malloc(sizeof(struct Node));
		t->data=A[i];
		t->next=NULL;
		last->next=t;
		last=t;
	}
	
}

void print_middle(struct Node *p)
{
	int num_nodes=0;
	struct Node* slow_pointer;
	struct Node* fast_pointer;
	slow_pointer=p;
	fast_pointer=p;
	
	while(fast_pointer->next!=NULL)
	{
		slow_pointer=slow_pointer->next;
		fast_pointer=fast_pointer->next->next;
	}
	
	cout<<"Middle "<<slow_pointer->data<<endl;
	
	
}
void display(struct Node* p)
{
	while(p!=NULL)
	{
		cout<<p->data<<endl;
		p=p->next;
	}
}


int main(int argc, char** argv) {
	int A[]={2,4,6,8,10,20,30,40,50};
	create(A,9);
	display(first);
	print_middle(first);
	return 0;
}
