def n2h(v, unit="",len=5):
  """ number to human representation"""
  prefix=['Y','Z','E','P','T','G','M','k','','m','u','n','p','f','a','z']
  sign=""
  if v<=0:
      sign="-"
      v=abs(v)
  order=1e24
  i=0
  if v<1e-21: return str(v)
  while True:
    if v>=order:
      v/=order
      vint=int(v)
      if vint>=100:
        l="%.0f"%(v)
      elif vint>10:
        l="%.1f"%(v)
      else:
        l="%.2f"%(v)
      l+=prefix[i]
      f="%%%ds"%len
      l=f%l
      if unit!="":
        l+=" [%s]"%unit
      l=sign+l
      return l
    order/=1e3
    i+=1
    
