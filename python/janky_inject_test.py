class A:
    def __init__(self):
        print('hi from A')


class B(A):
    def __init__(self):
        super().__init__()
        print('hi from B')


def inject(cls):
    __class__ = cls

    def inj(self):
        super().__init__()
        print('hi from B_inj')

    cls.__init__ = inj


inject(B)

B()
