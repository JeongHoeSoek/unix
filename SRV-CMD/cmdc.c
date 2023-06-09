#include <stdio.h>			// printf(), gets()
#include <string.h>			// strcmp(), strlen()
#include <stdlib.h>			// exit()
#include <unistd.h>			// write(), close(), unlink(), read(), write()
#include <fcntl.h>			// open() option
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>

#define SZ_STR_BUF		256

// 메시지 큐용으로 사용될 FIFO 파일
char *s_to_c = "fifo_s_to_c";
char *c_to_s = "fifo_c_to_s";

int  in_fd, out_fd;
int  len;
char cmd_line[SZ_STR_BUF];

// ****************************************************************
// 에러 원인을 화면에 출력하고 프로그램 강제 종료
// ****************************************************************
void 
print_err_exit(char *msg)
{
	perror(msg);
	exit(1);
}

// ****************************************************************
// 서버와 연결한다.
// ****************************************************************
void
connect_to_server()
{
	// 클라이언트 -> 서버 방향 통신용 FIFO 연결

	/* TODO: 여기에 클라이언트 -> 서버 방향 통신용 FIFO 연결용 코드를 삽입할 것 */
	out_fd = open(c_to_s, O_WRONLY);
	if (out_fd < 0)
			print_err_exit(c_to_s);
	// 서버 -> 클라이언트 방향 통신용 FIFO 연결

	/* TODO: 여기에 서버 -> 클라이언트 방향 통신용 FIFO 연결용 코드를 삽입할 것 */
	in_fd = open(s_to_c, O_RDONLY);
	if (in_fd < 0)
			print_err_exit(s_to_c);
}

// ****************************************************************
// 서버와 연결을 해제한다.
// ****************************************************************
void
dis_connect()
{
	close(out_fd);
	close(in_fd);
}

// ****************************************************************
// 키보드에서 읽어 서버로 보냄
// ****************************************************************
int
input_send()
{
	// 0: STDIN_FILENO
	if ((len = read(0, cmd_line, SZ_STR_BUF)) <= 0)	// 키보드에서 읽고
		return len;

	// 서버로 전송
	
	/* TODO: 여기에 서버로 전송하는 코드 삽입할 것 */
	if (write(out_fd, cmd_line, len) != len)
			return -1;
	return len;
}

// ****************************************************************
// 서버에서 메시지 받아 화면에 출력
// ****************************************************************
int
recv_output()
{
	// 서버에서 수신
	/* TODO: 여기에 서버로부터 수신하는 코드 삽입할 것 */
	len = read(in_fd, cmd_line, SZ_STR_BUF);
	if (len <= 0) return len;
	// 키보드에서 "exit"를 입력받아 서버로 전송하면 서버 프로그램이 먼저 종료한다.
	// 그러면 FIFO 연결이 끊어지고, 위 read()에서 0이 리턴되어 종료하게 된다. 
	if (write(1, cmd_line, len) != len)	// 화면에 출력, 1: STDOUT_FILENO
		return -1;
	return len;
}


// ****************************************************************
// 하나의 프로세스에서 반복적으로 아래를 수행
// 키보드 입력, 서버로 전송, 서버에서 수신, 화면에 출력을 순서적 처리함
// ****************************************************************


//부모 cmdc2: 반복하여 키보드에서 입력 받아 서버로 전송
void input_send_loop(void)
{
	while (1) 
	{
		if (input_send() <= 0)	 //키보드 입력 후
			break;			   	//서버로 전송
	}
}


//자식 cmdc2: 반복하여 서버에서 받은 데이터들 
//모니터로 출력

void recv_output_loop(void)
{
	while(1)	//서버로부터 받고 화면에 출력
	{
		if (recv_output() <= 0)
		break;
	}

}


static void sig_child(int sig)
{
	exit(0);	//자식 cmdc3가 종료하면 시그널을 받아 부모 cmdc3가 여기서 종료
}

void dual_process(void)
{
	pid_t pid;
	// 두개의 프로세스로 분리하여 각 프로세스가 입출력을 따로 담당
	if ((pid = fork()) < 0)	
		perror("fork"); 	//자기 복제 실패한 경우
	else if (pid > 0)		//부모 프로세스: 기보드 입력 받아 서버로 전송
	{
	//	signal(SIGINT, SIG_IGN);	컨트롤 c를 누르면, 신호를 무시하라
		signal(SIGCHLD, sig_child);	//자식이 먼저 죽으면, sig_child호출!
		close(in_fd);		//사용하기 않기 때문에 닫음
		input_send_loop();	//반복하여 키보드에서 입력 받아 서버로 전송
		wait(NULL);			//부모가 먼저 끝난 경우, 자식이 종료할 때까지 기다림

	}
	else		//자식 프로세스: 서버가 보내 준 메시지 받아 화면에 출력 
	{
	//	signal(SIGINT, SIG_IGN);	//컨트롤 c를 누르면, 신호를 무시하라.
		close(out_fd);		//사용하지 않기 때문에 닫음
		recv_output_loop();	//반복하여 서버에서 받은 데이터를 모니터로 출력
	}

}


// 문제점: 
//	클라이언트는 키보드에서 한줄 입력 받아 서버로 보내고 
//	서버에서 보낸 메시지를 한번 받아 화면에 출력한다. 
//	즉, 클라이언트는 한번 밖에 받지 않는다.
//  만약 서버가 여러 메시지를 여러 번에 걸쳐 클라이언트에 보내면 어떻게 될까?
//  예) 서버가 파일의 내용을 10번에 걸쳐 클라이언트에 보내 주면
//      클라이언트도 10번에 걸쳐 받으면 모두 다 받아 낼 수 있다.
//  그러나 서버가 몇 번에 걸쳐 보내 주는지 클라이언트가 사전에 알지 못하기 때문에 
//  클라이언트는 정확하게 모든 메시지를 받아 낼 수 없다.

// ****************************************************************
// main() 함수
// ****************************************************************
int
main(int argc, char *argv[])
{
	connect_to_server();	// 서버 프로그램 srv와 연결한다.
	dual_process();		// 반복수행: 입력, 서버로 전송, 서버에서 수신, 출력
	dis_connect(); 			// 서버 프로그램과 연결을 해제한다.
}

// 매번 cmdc, srv 프로그램 실행 전에 
// $ ps -u계정이름
// 위 명령어를 실행하여 기존에 실행시켰던 cmdc, srv 프로그램이 있는지 확인하고, 있으면
// $ kill -9 21032(process ID)
// 하여 기존 프로그램을 죽인 후 실행해야 한다.

