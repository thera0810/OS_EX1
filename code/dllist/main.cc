#include "dllist.h"
#include <iostream>

int main(){
    std::cout<<"test iostream"<<std::endl;
	printf("this is the main test!\n");
    DLList *list=new DLList();
    printf(list->IsEmpty()?"empty list\n":"not empty\n");
    dllFunc1(list,10);
    list->Show();
    dllFunc2(list,5);
    list->Show();
    return 0;
}

