#include <string.h>
#include <windows.h>
#include <iostream>
#include <fstream>
#ifndef SCHEDULE_H
#define SCHEDULE_H

enum STATUS {RUN, READY, WAIT}; // ���̵�����״̬�б�
// ���̿��ƿ���Ϊ30(���������30������)
const int PCB_LIMIT = 30;

typedef struct PCB { // ���̿��ƿ�
	int id; // ���̱�ʶPID
	char name[20]; // ������
	enum STATUS status; // ����״̬��RUN, READY, WAIT
	int flag; //Ϊ�˲��ظ���ʾ���������Ӵ˳�Ա
	HANDLE hThis; // ���̾��
	DWORD threadID; // �߳�ID
	int count; // ���̼ƻ�����ʱ�䳤�ȣ���ʱ��ƬΪ��λ
	struct PCB *next; // ָ��������л򻺳���(����PCB)����ָ��
} PCB, *pPCB;

// �������л�հ�PCB���еĹ����õĽṹ

typedef struct {
	pPCB head; // ����ָ��
	pPCB tail; // ��βָ��
	int pcbNum; // �����еĽ�����
} readyList, freeList, *pList;

HANDLE init();
void createProcess(char *name, int ax);
void scheduleProcess();
void removeProcess(char *name);
void fprintReadyList();
void printReadyList();
void printCurrent();
void stopAllThreads();
#endif

������
#include "schedule.h"
using namespace std;

readyList readylist; //����������еĹ���ṹ
pList pReadyList = &readylist; //����ýṹ��ָ��
freeList freelist; //�������PCB���еĹ���ṹ
pList pFreeList = &freelist; //����ýṹ��ָ��
PCB pcb[PCB_LIMIT]; //����30��PCB
pPCB runPCB; //ָ��ǰ���еĽ���PCB��ָ��
int pid = 0; //����id(����������)
//�ٽ�������
CRITICAL_SECTION cs_ReadyList; //���ڻ�����ʾ�������
CRITICAL_SECTION cs_SaveInfo; //���ڻ�����ʱ�����Ϣ���ļ�
//����ļ�
extern ofstream log;
extern volatile bool exiting; //��exiting=trueʱ�������
//��ʼ�����̿��ƿ�
void initialPCB(pPCB p) {
	p->id = 0; //����id
	strcpy(p->name, "NoName"); //��������
	p->status = WAIT; //����״̬
	p->next = NULL; //PCB��nextָ��
	p->hThis = NULL; //���̾��
	p->threadID = 0; //�߳�id
	p->flag = 1; //��ʼʱ�����߳���ʾ��Ϣ
	p->count = 0; //���̼ƻ�����ʱ��
}
//�ӿհ�PCB����ȡһ���н��̿��ƿ�
pPCB getPcbFromFreeList() {
	pPCB freePCB = NULL;
	if (pFreeList->head != NULL && pFreeList->pcbNum > 0) {
		freePCB = pFreeList->head;
		pFreeList->head = pFreeList->head->next;
		pFreeList->pcbNum--;
	}
	return freePCB;
}

//�ͷ�PCBʹ֮�������PCB����
void returnPcbToFreeList(pPCB p) {
	if (pFreeList->head == NULL) { //����ǰ����PCB����Ϊ��
		pFreeList->head = p;
		pFreeList->tail = p;
		p->next = NULL;
		pFreeList->pcbNum++;
	} else { //���հ�PCB���в��գ����ͷŵ�PCB�������
		p->next = pFreeList->head;
		pFreeList->head = p;
		pFreeList->pcbNum++;
	}
}

// ģ���û����̵��߳�ִ֮�д���
DWORD WINAPI processThread(LPVOID lpParameter) {
	pPCB currentPcb = (pPCB)lpParameter;
	while (true) {
		if (currentPcb->flag == 1) { //�����Ⱥ��һ�����У�����ʾ��Ϣ
			currentPcb->flag = 0;
			EnterCriticalSection(&cs_SaveInfo);
			//"������������"��Ϣ���浽�ļ�
			log << "Process " << currentPcb->id << ':' << currentPcb->name << " is running............" << endl;
			LeaveCriticalSection(&cs_SaveInfo);//�뿪�ٽ���
		}
		Sleep(800); // �ȴ�800ms
	}
	return 1;
}

// �����̵߳�ִ�д���
DWORD WINAPI scheduleThread(LPVOID lpParameter) {
	// pList preadyList=(pList)lpParameter;//ʵ���ϴ˲���������
	//ѭ�����ý��̵��Ⱥ�����ֱ��exiting=trueΪֹ
	while (!exiting) { //��exiting=false,��ѭ��ִ�е��ȳ���
		scheduleProcess();
	}
	stopAllThreads();//��exiting=true����������н���(�߳�)
	return 1;
}

// ��ʼ������
HANDLE init() { //������ȷִ�к󣬷��ص����̵߳ľ��
	pPCB p = pcb; //ָ���һ��PCB
	//�������г�ʼ��Ϊ��(��ʼ�������ṹ)
	pReadyList->head = NULL;
	pReadyList->tail = NULL;
	pReadyList->pcbNum = 0;
	//���ж��г�ʼ��Ϊ��(��ʼ�������ṹ)
	pFreeList->head = &pcb[0];
	pFreeList->tail = &pcb[PCB_LIMIT - 1];
	pFreeList->pcbNum = PCB_LIMIT;
	//���ɿ���PCB����
	for (int i = 0; i < PCB_LIMIT - 1; i++) { //PCB_LIMIT����ͷ�ļ��ж���Ϊ30
		initialPCB(p);
		p->next = &pcb[i + 1];
		p++;
	}

	initialPCB(p);
	pcb[PCB_LIMIT - 1].next = NULL;
	InitializeCriticalSection(&cs_ReadyList); //��ʼ���ٽ�������
	InitializeCriticalSection(&cs_SaveInfo);
	exiting = false; //ʹ���ȳ��򲻶�ѭ��
	// �������ȳ���ļ���߳�
	HANDLE hSchedule; //��������̵߳ľ��
	hSchedule = CreateThread(
	                NULL, //���صľ�����ܱ����̼̳߳�
	                0, //���̶߳�ջ��С�����߳���ͬ
	                scheduleThread, //���߳�ִ�д˲���ָ���ĺ����Ĵ���
	                pReadyList, //���ݸ�����scheduleThread�Ĳ���(�������д˲���ʵ����û���ô�)
	                0, //���̵߳ĳ�ʼ״̬Ϊ����״̬
	                NULL //���´����̵߳�id������Ȥ
	            );
	//Ԥ�ȴ���6������
	char pName[6] = "p00";
	for (int i = 0; i < 6; i++) {
		pName[2] = '0' + i;
		createProcess(pName, 10); //addApplyProcess(pName,10);
	}
	return hSchedule;
}

// ��������(�˺�������API����CreateProcess)
void createProcess(char *name, int count) {
	EnterCriticalSection(&cs_ReadyList); //׼�������ٽ���
	if (pFreeList->pcbNum > 0) { // �������ڴ������̵Ŀհ�PCB
		pPCB newPcb = getPcbFromFreeList(); // �ӿհ�PCB���л�ȡһ���հ�PCB
		newPcb->status = READY; // �½���״̬Ϊ"READY"
		strcpy(newPcb->name, name); // ��д�½��̵�����
		newPcb->count = count; // ��д�½��̵�����ʱ��
		newPcb->id = pid++; // ����id
		newPcb->next = NULL; // ��д��PCB������ָ��
		//���������пգ�����PCBΪReadyList�ĵ�һ�����
		if (pReadyList->pcbNum == 0) {
			pReadyList->head = newPcb;
			pReadyList->tail = newPcb;
			pReadyList->pcbNum++;
		} else { //���򣬾������в��գ���PCB�����������β��
			pReadyList->tail->next = newPcb;
			pReadyList->tail = newPcb;
			pReadyList->pcbNum++;
		}
		cout << "New Process Created, Process ID:" << newPcb->id << ", Process Name:" << newPcb->name << ", Process Length:" <<
		     newPcb->count << endl;
		cout << "Current ReadyList is:" << endl;
		printReadyList();
		//����Ϣ�ļ���������Ϣ�Ա�鿴����ִ�й���
		EnterCriticalSection(&cs_SaveInfo);
		log << "New Process Created, Process ID:" << newPcb->id << ", Process Name:" << newPcb->name << ", Process Length:" <<
		    newPcb->count << endl;
		log << "Current ReadyList is:" << endl;
		fprintReadyList();
		LeaveCriticalSection(&cs_SaveInfo);
		//�����û��̣߳���ʼ״̬Ϊ��ͣ
		newPcb->hThis = CreateThread(NULL, 0, processThread,
		                             newPcb, CREATE_SUSPENDED, &(newPcb->threadID));
	} else { //����PCB����
		cout << "New process intend to append. But PCB has been used out!" << endl;
		EnterCriticalSection(&cs_SaveInfo);
		log << "New process intend to append. But PCB has been used out!" << endl;
		LeaveCriticalSection(&cs_SaveInfo);
	}
	LeaveCriticalSection(&cs_ReadyList); // �˳��ٽ���
}

// ���̵���
void scheduleProcess() {
	EnterCriticalSection(&cs_ReadyList);
	if (pReadyList->pcbNum > 0) { // �����������н��������
		runPCB = pReadyList->head; // ���ȳ���ѡ����������е�һ������
		pReadyList->head = pReadyList->head->next; //�޸ľ������е�ͷָ��
		if (pReadyList->head == NULL) // �����������ѿգ������޸���βָ��
			pReadyList->tail = NULL;
		pReadyList->pcbNum--; // �������нڵ�����1
		runPCB->count--; // �½���ʱ��Ƭ����1
		runPCB->flag = 1; //����ÿ�α����ȣ�ֻ��ʾ1��
		EnterCriticalSection(&cs_SaveInfo);
		log << "Process " << runPCB->id << ':' << runPCB->name << " is to be scheduleed." << endl;
		LeaveCriticalSection(&cs_SaveInfo);
		ResumeThread(runPCB->hThis);// �ָ��߳�(����)���ڱ�������ʵ�����������߳�����
		runPCB->status = RUN; // ����״̬����Ϊ"RUN"
		// ʱ��ƬΪ1s
		Sleep(1000); // �ȴ�1���ӣ��ô�ģ��(ʱ��ƬΪ1s��)��ʱ�ж�
		EnterCriticalSection(&cs_SaveInfo);
		log << "\nOne time slot used out!\n" << endl;
		LeaveCriticalSection(&cs_SaveInfo);
		runPCB->status = READY;
		SuspendThread(runPCB->hThis); // ��ǰ���н��̱�����
		// �жϽ����Ƿ��������
		if (runPCB != NULL && runPCB->count <= 0) {
			cout << "\n****** Process " << runPCB->id << ':' << runPCB->name << " has finished. ******" << endl;
			cout << "Current ReadyList is:" << endl;
			printReadyList();
			cout << "COMMAND>";
			cout << flush;
			EnterCriticalSection(&cs_SaveInfo);
			log << "****** Process " << runPCB->id << ':' << runPCB->name << " has finished. ******\n" << endl;
			log << "Current ReadyList is:" << endl;
			fprintReadyList();
			log << flush;
			LeaveCriticalSection(&cs_SaveInfo);
			// ��ֹ����(�߳�)
			if (!TerminateThread(runPCB->hThis, 1)) {
				//����ֹ�߳�ʧ�ܣ������������Ϣ��������������
				EnterCriticalSection(&cs_SaveInfo);
				log << "Terminate thread failed! System will abort!" << endl;
				LeaveCriticalSection(&cs_SaveInfo);
				exiting = true; //��������
			} else { //��ֹ�̳в�����ȷִ��
				CloseHandle(runPCB->hThis);
				//��ֹ���̵�PCB�ͷŵ��հ�PCB��������
				returnPcbToFreeList(runPCB);
				runPCB = NULL;
			}
		} else if (runPCB != NULL) { //����δ������ϣ���������������
			if (pReadyList->pcbNum <= 0) { //��������Ϊ��ʱ�Ĵ���
				pReadyList->head = runPCB;
				pReadyList->tail = runPCB;
			} else { //��������Ϊ����ʱ��ԭ���н��̵�PCB�ӵ���������β��
				pReadyList->tail->next = runPCB;
				pReadyList->tail = runPCB;
			}
			runPCB->next = NULL;
			runPCB = NULL;
			pReadyList->pcbNum++; //�������н�������1
		}
	} else if (pReadyList != NULL) { // ��վ�������
		pReadyList->head = NULL;
		pReadyList->tail = NULL;
		pReadyList->pcbNum = 0;
	}
	LeaveCriticalSection(&cs_ReadyList);
}

// ��������
void removeProcess(char *name) {
	pPCB removeTarget = NULL;
	pPCB preTemp = NULL;
	EnterCriticalSection(&cs_ReadyList); //������ʾ�������
	// ���������ǵ�ǰ���н���
	if (runPCB != NULL && strcmp(name, runPCB->name) == 0 ) {
		removeTarget = runPCB;
		if (!(TerminateThread(removeTarget->hThis, 1))) {
			cout << "Terminate thread failed! System will abort!" << endl;
			EnterCriticalSection(&cs_SaveInfo);
			log << "Terminate thread failed! System will abort!" << endl;
			LeaveCriticalSection(&cs_SaveInfo);
			exit(0); //��������
		} else { // ���������ɹ�ʱ
			CloseHandle(removeTarget->hThis); //�رս��̾��
			returnPcbToFreeList(removeTarget); //�ý��̵�PCB�������PCB����
			runPCB = NULL;
			//��ʾ�����ѳ�������Ϣ
			cout << "\nProcess " << removeTarget->id << ':' << removeTarget->name << " has been removed." << endl;
			cout << "Current ReadyList is:\n";
			printReadyList();
			//ͬʱ�������ѳ�������Ϣ���浽�ļ�
			EnterCriticalSection(&cs_SaveInfo);
			log << "\nProcess " << removeTarget->id << ':' << removeTarget->name << " has been removed." << endl;
			log << "Current ReadyList is:\n";
			fprintReadyList();
			log << flush;
			LeaveCriticalSection(&cs_SaveInfo);
			LeaveCriticalSection(&cs_ReadyList);
			return;
		}
	}
	// �����ھ���������Ѱ��Ҫ�����Ľ���
	if (pReadyList->head != NULL) {
		removeTarget = pReadyList->head;
		while (removeTarget != NULL) {
			if (strcmp(name, removeTarget->name) == 0) { //�ҵ�Ҫ�����Ľ���
				if (removeTarget == pReadyList->head) { //�Ǿ��������еĵ�һ������
					pReadyList->head = pReadyList->head->next;
					if (pReadyList->head == NULL)
						pReadyList->tail = NULL;
				} else { // �ҵ��Ĳ��Ǿ��������е�һ������
					preTemp->next = removeTarget->next;
					if (removeTarget == pReadyList->tail)
						pReadyList->tail = preTemp;
				}
				if (!TerminateThread(removeTarget->hThis, 0)) { //ִ�г������̵Ĳ���
					//��������ʧ��ʱ�������Ϣ
					cout << "Terminate thread failed! System will abort!" << endl;
					EnterCriticalSection(&cs_SaveInfo);
					log << "Terminate thread failed! System will abort!" << endl;
					LeaveCriticalSection(&cs_SaveInfo);
					LeaveCriticalSection(&cs_ReadyList);
					exit(0); //��������
				}
				//���������ɹ���Ĵ���
				CloseHandle(removeTarget->hThis);
				returnPcbToFreeList(removeTarget);
				pReadyList->pcbNum--;
				cout << "Process " << removeTarget->id << ':' << removeTarget->name << " has been removed." << endl;
				cout << "currentreadyList is:" << endl;
				printReadyList();
				EnterCriticalSection(&cs_SaveInfo);
				log << "Process " << removeTarget->id << ':' << removeTarget->name << " has been removed." << endl;
				log << "currentreadyList is:" << endl;
				fprintReadyList();
				log << flush;
				LeaveCriticalSection(&cs_SaveInfo);
				LeaveCriticalSection(&cs_ReadyList);
				return;
			} else { //δ�ҵ���������
				preTemp = removeTarget;
				removeTarget = removeTarget->next;
			}
		}
	}
	LeaveCriticalSection(&cs_ReadyList);
	cout << "Sorry, there's no process named " << name << endl;
	return;
}

// ���ļ��д�ӡ����������Ϣ
void fprintReadyList() {
	pPCB tmp = NULL;
	tmp = pReadyList->head;
	if (tmp != NULL)
		for (int i = 0; i < pReadyList->pcbNum; i++) {
			log << "--" << tmp->id << ':' << tmp->name << "--";
			tmp = tmp->next;
		} else
		log << "NULL";
	log << endl << endl;
}

// ���׼�����ӡ����������Ϣ
void printReadyList() {
	pPCB tmp = NULL;
	tmp = pReadyList->head;
	if (tmp != NULL)
		for (int i = 0; i < pReadyList->pcbNum; i++) {
			cout << "--" << tmp->id << ':' << tmp->name << "--";
			tmp = tmp->next;
		} else
		cout << "NULL";
	cout << endl;
}

// ��ӡ��ǰ���н�����Ϣ
void printCurrent() {
	if (runPCB != NULL)
		cout << "Process " << runPCB->name << " is running..." << endl;
	else
		cout << "No process is running." << endl;
	cout << "Current readyList is:" << endl;
	printReadyList();
}

// �����������߳�
void stopAllThreads() {
	if (runPCB != NULL) {
		TerminateThread(runPCB->hThis, 0);
		CloseHandle(runPCB->hThis);
	}
	// �������о��������е��߳�
	pPCB q, p = pReadyList->head;
	while (p != NULL) {
		if (!TerminateThread(p->hThis, 0)) {
			cout << "Terminate thread failed! System will abort!" << endl;
			exit(0); //��������
		}
		CloseHandle(p->hThis);
		q = p->next;
		returnPcbToFreeList(p);
		p = q;
	}
}
ofstream log;  //������̵�����Ϣ���ļ�
volatile bool exiting; //�Ƿ��˳�����

void helpInfo() {
	cout << "************************************************\n";
	cout << "COMMAND LIST:\n";
	cout << "create process_name process_length (create p0 8)\n";
	cout << "\t append a process to the process list\n";
	cout << "remove process_name (remove p0)\n";
	cout << "\t remove a process from the process list\n";
	cout << "current\t show current runprocess readyList\n";
	cout << "exit\t exit this simulation\n";
	cout << "help\t get command imformation\n";
	cout << "************************************************\n\n";
}

int main() {
	char name[20] = {'\0'};
	HANDLE hSchedule; //�����̵߳ľ��
	log.open("Process_log.txt");
	helpInfo();
	hSchedule = init(); //hSchedule�ǵ��ȳ���ľ��
	if (hSchedule == NULL) {
		cout << "\nCreate schedule-process failed. System will abort!" << endl;
		exiting = true;
	}
	char command[30] = {0};
	while (!exiting) {
		cout << "COMMAND>";
		cin >> command;
		if (strcmp(command, "exit") == 0)
			break;
		else if (strcmp(command, "create") == 0) {
			char name[20] = {'\0'};
			int time = 0;
			cin >> name >> time;
			createProcess(name, time);
		} else if (strcmp(command, "remove") == 0) {
			cin >> name;
			removeProcess(name);
		} else if (strcmp(command, "current") == 0)
			printCurrent();
		else if (strcmp(command, "help") == 0)
			helpInfo();
		else
			cout << "Enter help to get command information!\n";
	}
	exiting = true;
	if (hSchedule != NULL) {
		//���޵ȴ���ֱ��Schedule����(�߳�)��ֹΪֹ
		WaitForSingleObject(hSchedule, INFINITE);
		CloseHandle(hSchedule);
	}
	log.close();
	cout << "\n******* End *******\n" << endl;
	return 0;
}