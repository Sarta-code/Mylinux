public class Test{
    public static void main(String arg[]){
	int x;
	int a[] = {0,0,0,0,0,0};
	calculate(a,a[5]);
	System.out.println("the value ofa[0] is "+a[0]);
	System.out.println("the value ofa[5] is "+a[5]);
    }

    static int calculate(int x[],int y){
	for(int i = 1;i < x.length;i++){
	    if(y < x.length){
		x[i] = x[i-1] + 1;
	    }
	}
	return x[0];
    }
}
