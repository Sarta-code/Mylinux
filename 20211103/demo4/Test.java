public class Test {
    public static void main(String arg[]){
	char temp = 'C';
	switch(temp){
	    case 'A':
		System.out.println("优秀");
		break;
	    case 'B':
	    case 'C':
		System.out.println("良好!");
		break;
	    case 'D':
		System.out.println("及各！");
		break;
	    case 'E':
		System.out.println("你需要在努力努力！");
		break;
	    default :
		System.out.println("未知成绩!");
	}
    }
}
