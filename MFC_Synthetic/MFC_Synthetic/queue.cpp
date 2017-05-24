#include "stdafx.h"
#include "MFC_Synthetic.h"
#include "MFC_SyntheticDlg.h"
#include "afxdialogex.h"
#include <io.h>

/************************
큐
************************/


void InitQueue(Queue *queue)
{
	queue->front = queue->rear = NULL; //front와 rear를 NULL로 설정
	queue->count = 0;//보관 개수를 0으로 설정
}

int IsEmpty(Queue *queue)
{
	return queue->count == 0;    //보관 개수가 0이면 빈 상태
}

void Enqueue(Queue *queue, int data, int index)
{
	node *now = (node *)malloc(sizeof(node)); //노드 생성
	//데이터 설정
	now->timeTag = data;
	now->indexOfSegmentArray = index;
	now->next = NULL;

	if (IsEmpty(queue))//큐가 비어있을 때
	{
		queue->front = now;//맨 앞을 now로 설정       
	}
	else//비어있지 않을 때
	{
		queue->rear->next = now;//맨 뒤의 다음을 now로 설정
	}
	queue->rear = now;//맨 뒤를 now로 설정   
	queue->count++;//보관 개수를 1 증가
}

node Dequeue(Queue *queue)
{
	int data = 0;
	node nowAddress;
	node *now;
	now = &nowAddress;
	if (IsEmpty(queue))//큐가 비었을 때
	{
		return *now;
	}
	now = queue->front;//맨 앞의 노드를 now에 기억
	data = now->timeTag;//반환할 값은 now의 data로 설정
	queue->front = now->next;//맨 앞은 now의 다음 노드로 설정
	queue->count--;//보관 개수를 1 감소
	return *now;
}