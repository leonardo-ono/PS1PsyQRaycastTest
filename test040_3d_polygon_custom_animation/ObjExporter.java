import java.util.ArrayList;
import java.util.List;
import java.util.Scanner;

public class ObjExporter {
    
    private static double[][] vertices, sts;
    private static int[][] faces;

    public static void main(String[] args) {
        if (1==0){
            System.out.println("teste2");
            return;
        } 
        
        int textureWidth = 255;
        int textureHeight = 255;

        try ( Scanner sc = new Scanner(
                ObjExporter.class.getResourceAsStream("run.obj")) ) {
            
            List<int[]> facesTmp = new ArrayList<>();
            List<double[]> stsTmp = new ArrayList<>();
            List<double[]> verticesTmp = new ArrayList<>();
            sc.useDelimiter("[ /\n]");
            while (sc.hasNext()) {
                String token = sc.next();
                if (token.equals("v")) {
                    verticesTmp.add(new double[] { sc.nextDouble(), sc.nextDouble(), sc.nextDouble() } );
                }
                else if (token.equals("vt")) {
                    double a = sc.nextDouble();
                    double b = sc.nextDouble();
                    if (a < 0) a += 1.0;
                    if (b < 0) b += 1.0;
                    stsTmp.add(new double[] { a * textureWidth
                            , (1 - b) * textureHeight } );
                }
                else if (token.equals("f")) {
                    facesTmp.add( new int[] { sc.nextInt() - 1, sc.nextInt() - 1
                      , sc.nextInt() - 1, sc.nextInt() - 1
                      , sc.nextInt() - 1, sc.nextInt() - 1 } );
                }
            }
            faces = facesTmp.toArray(new int[0][0]);
            sts = stsTmp.toArray(new double[0][0]);
            vertices = verticesTmp.toArray((new double[0][0]));
        }
        System.out.println("#define VERTICES_COUNT " + vertices.length);        
        System.out.println("#define UVS_COUNT " + sts.length);        
        System.out.println("#define FACES_COUNT " + faces.length);

        // vertices
        double scale = 3072;
        System.out.println("SVECTOR vertices[VERTICES_COUNT] = {");
        for (int i = 0; i < vertices.length; i++) {
            double[] vertex = vertices[i];
            int vx = (int) (vertex[0] * scale);
            int vy = (int) (vertex[1] * -scale);
            int vz = (int) (vertex[2] * scale);
            System.out.println("   { " + vx + ", " + vy + ", " + vz + " },"); 
        }
        System.out.println("};");

        // uvs
        System.out.println("SVECTOR uvs[UVS_COUNT] = {");
        for (int i = 0; i < sts.length; i++) {
            double[] uv = sts[i];
            int u = (int) uv[0];
            int v = (int) uv[1];
            System.out.println("   { " + u + ", " + v + ", 0 },");
        }
        System.out.println("};");
        
        // faces
        System.out.println("int faces[FACES_COUNT][3][2] = {");
        for (int i = 0; i < faces.length; i++) {
            int[] face = faces[i];
            System.out.println("   " + face[0] + ", " + face[1] + ", " + face[2] + ", " + face[3] + ", " + face[4] + ", " + face[5] + ", ");
        }
        System.out.println("};");
        
        
    }

}