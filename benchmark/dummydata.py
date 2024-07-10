class DummyData:
    def __init__(self, val):
        self.astr = f"a string {val}"
        self.num = 1704 + val
        self.adict = {
            "val": 17 + val,
            "str": f"string inside a dictionary {val}"
        }
        self.atup = (4, 17, 16, val, 15, 30)
        self.aset = set([1, 2, 9, 6, val])
    
    def __str__(self):
        return f"{self.astr}, {self.num}, {self.adict}, {self.atup}, {self.aset}"
    
    __repr__ = __str__