bool fileTest(const char* path)
{
  static Long_t *id, *size, *flags, *mt;
  bool ret = gSystem->GetPathInfo(path, id, size, flags, mt) == 0;
  return ret;
}
