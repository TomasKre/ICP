void init( void )
{
  //...
    //transparency blending function
    glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
  //...
}

int main( int argc, char * argv[])
{
    ...
    init();
    ...
    while (1)
    {

        //...
        // draw all non-transparent in any order
        //...
      
    	//semi-transparent object, colour through Phong model
        glEnable( GL_BLEND );                         // enable blending
        glDisable( GL_CULL_FACE );                    // no polygon removal
        glDepthMask(GL_FALSE);                        // set Z to read-only
            
        //draw transparent (from far to near)
        //...
      
        glDisable( GL_BLEND );
        glEnable( GL_CULL_FACE );                    
        glDepthMask(GL_TRUE);
        
        //end frame
        glfwSwapBuffers();
    }
    ...

}
