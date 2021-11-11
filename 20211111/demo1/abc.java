public class abc {
    public static void main(String arg[]){
	AB s = new AB("Hello!","I love java.");
	System.out.println(s.toString());
    }
}
class AB{
    String s1;
    String s2;
    public AB(String str1,String str2){
	s1 = str1;
	s2 = str2;
    }
    public String toString(){
	return s1+s2;
    }
}
