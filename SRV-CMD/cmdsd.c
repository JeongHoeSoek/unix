#include <stdio.h>			// printf(), gets()
#include <string.h>			// strcmp(), strlen()
#include <stdlib.h>			// exit()
#include <unistd.h>			// write(), close(), unlink(), read(), write()
#include <fcntl.h>			// open() option
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>

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
// 클라이언트와 연결한다.
// ****************************************************************

void
connect_to_client()
{

	//서버와 클라이언트가 통신할 두개의 단방향 FIFO파일을 생성
	mkfifo(c_to_s, 0600);	//client -> 서버로 전송할 FIFO 파일
	mkfifo(s_to_c, 0600);	//서버 -> 클라이언트로 전송할 FIFO 파일

	//daemonize()에서 모든 파일을 close()했기 때문에 open된 파일이 하나도 없음
	//이 상태에서 파일을 open하면 파일 핸들 값은 0,1,2,3, 순서로 할당됨
	
	// 클라이언트 -> 서버: 한방향 통신용 FIFO 연결
	in_fd = open(c_to_s, O_RDONLY);		//in fd: 0임
	if (in_fd < 0)
			print_err_exit(c_to_s);


	out_fd = open(s_to_c, O_WRONLY);	//out_fd: 1임
		if (out_fd < 0)
				print_err_exit(s_to_c);

	dup2(out_fd, 2); //out_fd:1를 핸들 2에 복제함, 따라서 2도 s_to_c를 지칭함
}

// ****************************************************************
// 클라이언트와 연결을 해제한다.
// ****************************************************************
void
dis_connect()
{

	close(0);
	close(1);
	close(2);
//클라이언트 cmdc와 연결된 메시지 큐와의 접속을 닫
}


void duplicate_IO(){
	
	// 서버의 표준 입력을 수신용 FIFO 파일 핸들로 변경하고
	// 표준 출력, 에러출력을 송신용 FIFO 파일 핸들로 변경한다.
	dup2(in_fd, 0);
	dup2(out_fd, 1);
	dup2(out_fd, 2);

	//표준 입출력 버퍼를 없앤다. 입출력이 중간에 버퍼링 되지 않고 바로 I/O된다.
	setbuf(stdin, NULL);
	setbuf(stdout, NULL);
	//stderr는 기본적으로 버퍼링 되지 않음음

	// 표준 입출력 0,1,2 핸들에 복제 되었으므로 이제 in_fd, out_fd는 필요없음.
	dis_connect();  //close(in_fd); close(out_fd);

	//***표준 I/O 핸들 0,1,2를 통해 클라이언트와 연결되어 있으므로 
	//close()해도 실제로 클라이언트와의 전속이 끊어지는 것이 아니다.
	
}




// ****************************************************************
// main() 함수
// ****************************************************************
int
main(int argc, char *argv[])
{
	close(0); close(1); close(2); //표준입출력을 모두 닫음


	while (1)
	{
	connect_to_client(); 
	//fork를 호출해서 현재 실행중인 cmdsd를 복제,
	//자식 프로세스 cmds를 생성
	pid_t pid = fork();
	if (pid < 0) //에러 발생
		print_err_exit("cmdsd: fork()");
	//자식 프로세스는 기존의 cmd 프로그램으로 프로세스 이미지(실행함수들)로 교체함)
	else if (pid == 0) //자식 프로세스: execl() 후 계속 cmdc와 통신함(연결유지)
	execl("../cmd/cmd", "cmd", NULL);
	//부모 프로세스는 자식(cmd)이 종료될 때까지 대기(자식은 cmd에 연결되어 있음)
	else
		{ //pid > 0일 경우, 즉 부모프로세스
		dis_connect();	//cmdc와 연결된 접속을 모두 닫음(자식이 대신 통신하므로)
		waitpid(pid,NULL,0);	// 자식(pid)이 종료될 때까지 대기
		sleep(2);				// cmdc가 완전히 종료되길 잠시 기다림
		}

	}

}

// 매번 cmdc, srv 프로그램 실행 전에 
// $ ps -u계정이름
// 위 명령어를 실행하여 기존에 실행시켰던 cmdc, srv 프로그램이 있는지 확인하고, 있으면
// $ kill -9 21032(21032는 죽일 프로세스의 process ID)
// 하여 기존 프로그램을 죽인 후 실행해야 한다.
