

package exp1;
public interface Area {
    public abstract double calculateArea();
}
package exp1;
public class MyRectangle implements Area {
    protected double a;
    protected double b;
    public MyRectangle(double a,double b)
    {
        this.a=a;
        this.b=b;
    }
    public double calculateArea()
    {
        return this.a*this.b;
    }
    public String toSting()
    {
        return "该矩形长为："+a+"，宽为："+b+"，面积为："+calculateArea();
    }
    public static void main(String args[])
    {
        System.out.println(new MyRectangle(25,4).toSting());
    }
}

package exp1;

public class MyCircle implements Area {
    protected double r;
    public MyCircle(double r)
    {
        this.r = r;
    }
    public double calculateArea()
    {
        return Math.PI*this.r*this.r;
    }
    public String toSting()
    {
        return "该圆形半径为："+r+"，面积为："+calculateArea();
    }
    public static void main(String args[])
    {
        System.out.println(new MyCircle(10).toSting());
    }
}
