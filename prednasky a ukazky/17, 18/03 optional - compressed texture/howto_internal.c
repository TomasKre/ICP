if (glewIsSupported("GL_ARB_texture_compression"))
{
	GLint num_compressed_format;

	// get list of supported formats
	glGetIntegerv(GL_NUM_COMPRESSED_TEXTURE_FORMATS_ARB, &num_compressed_format);
	if (num_compressed_format > 0)
	{
		GLint compressed, internalformat, compressed_size;

		std::cout << "COMPRESSION supported, tot. available formats: " << num_compressed_format << std::endl;

		// try to load compressed texture
		glHint(GL_TEXTURE_COMPRESSION_HINT, GL_FASTEST);
		//glHint(GL_TEXTURE_COMPRESSION_HINT, GL_NICEST);

		glTexImage2D(GL_TEXTURE_2D, 0, GL_COMPRESSED_RGB, image.cols, image.rows, 0, GL_BGR, GL_UNSIGNED_BYTE, image.data);

		// Is it now really compressed? Did we succeed?
		glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_COMPRESSED_ARB, &compressed);
		// if the compression has been successful
		if (compressed == GL_TRUE)
		{
			glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_INTERNAL_FORMAT, &internalformat);
			glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_COMPRESSED_IMAGE_SIZE_ARB, &compressed_size);
			std::cout << "ORIGINAL: " << image.total()*image.elemSize() << " COMPRESSED: " << compressed_size << " INTERNAL FORMAT: " << internalformat << std::endl;
		}
	}
    else
    { std::cout << "Wtf?" << std::endl; }
}
else // compression not supported
{
	// load uncompressed
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, image.cols, image.rows, 0, GL_BGR, GL_UNSIGNED_BYTE, image.data);
}
