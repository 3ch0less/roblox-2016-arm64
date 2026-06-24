#ifdef VERTEX_SHADER
void main() {
    gl_Position = shadowModelView * shadowProjection * vec4(position, 1.0);
}
#endif

#ifdef FRAGMENT_SHADER
void main() {
    gl_FragColor = vec4(gl_FragCoord.z, 0.0, 0.0, 1.0);
}
#endif
