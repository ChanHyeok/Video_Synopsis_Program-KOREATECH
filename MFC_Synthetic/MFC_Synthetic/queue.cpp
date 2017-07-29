// check point
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

void Enqueue(Queue *queue, segment data)
{
	//노드 생성
	node *now = new node;
	
	//데이터 설정
	now->segment_data = data;

	//노드 다음 포인터 초기화 
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

	// 임시로 생성한 node 메모리 해제
}

node Dequeue(Queue *queue)
{
	segment data;
	node nowAddress, *now;
	now = &nowAddress;
	if (IsEmpty(queue))//큐가 비었을 때
	{
		return *now;
	}
	now = queue->front;//맨 앞의 노드를 now에 기억
	data = now->segment_data;//반환할 값은 now의 data로 설정
	queue->front = now->next;//맨 앞은 now의 다음 노드로 설정
	queue->count--;//보관 개수를 1 감소
	return *now;
}


/************************
prevHumanDetectedVector에 사용
component vector에 대한 고정 크기 5인 원형큐의 연산들 정의
************************/

void InitComponentVectorQueue(ComponentVectorQueue *componentVectorQueue) {
	printf("Init Queue\n");
	componentVectorQueue->front = 0;
	componentVectorQueue->rear = 0;
}// 앞 뒤를 나타내는 포인터 두개를 같은 자리에 둠으로 비어있는 큐 생성

bool IsComponentVectorQueueEmpty(ComponentVectorQueue *componentVectorQueue) {
	// 앞 뒤 포인터가 같으면 true
	if (componentVectorQueue->front == componentVectorQueue->rear)
		return true;
	else
		return false;
}

bool IsComponentVectorQueueFull(ComponentVectorQueue *componentVectorQueue) {
	// 큐의 앞/뒤 포인터의 차이가 1이 날 경우
	if (NEXT(componentVectorQueue->rear) == componentVectorQueue->front)
		return true;
	else
		return false;
}

// 큐 추가 연산
void PutComponentVectorQueue(ComponentVectorQueue *componentVectorQueue, vector<component> componentVector) {
	// 큐가 가득 차 있을 경우 함수 탈출
	if (IsComponentVectorQueueFull(componentVectorQueue))
		return;

	// 데이터 추가
	componentVectorQueue->buf[componentVectorQueue->rear] = componentVector;

	// 모듈러 연산(포인터의 제일 끝에 도달할 경우 가장 앞으로 전환
	componentVectorQueue->rear = NEXT(componentVectorQueue->rear);
}

// 큐 삭제 연산 (삭제한 데이터는 버림)
void RemoveComponentVectorQueue(ComponentVectorQueue *componentVectorQueue) {
	// 큐가 비었을 경우 삭제연산이 의미가 없기떄문에 함수 탈출
	if (IsComponentVectorQueueEmpty(componentVectorQueue))
		return;

	// 큐의 전방 위치 증가 및 모듈러 연산
	componentVectorQueue->front = NEXT(componentVectorQueue->front);
}

// point번째 component 데이터를 반환 
vector<component> GetComponentVectorQueue(ComponentVectorQueue *componentVectorQueue, int point) {
	return componentVectorQueue->buf[point];
}
