public class abc {
    public static void main(String arg[]){
	System.out.println("a="+a+"\nb="+b);
    }
}
class SubClass extends SuperClass{
    //没有找到 SuperClass类，所以无法完成继承
    int c;

    SubClass(int aa,int bb,int cc){
	super(aa,bb);
	//没有找到super()方法
	c = cc;
    }

}
class SubSubClass extends SubClass {
    int a;
    SubSubClass(int aa,int bb,int cc){
	super(aa,bb,cc);
	a = aa+bb+cc;
    }
    void shou(){
	System.out.println("a="+a+"\nb="+b+"\nc="+c);
    }
}
