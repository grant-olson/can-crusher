import yaml
from os import path

class Config:
  def __init__(self):
    self.config = {}

  def load(self,config_file=".config"):
    if path.exists(config_file):
      self.config = yaml.load(open(config_file,"r").read(),Loader=yaml.Loader)
    else:
      self.config = {}

  def save(self,config_file=".config"):
    with open(config_file,"w") as f:
      f.write(yaml.dump(self.config,Dumper=yaml.Dumper))

  def __getitem__(self,k):
    return self.config[k]

  def __setitem__(self,k,v):
    self.config[k] = v

  def __contains__(self,k):
    return k in self.config

      
