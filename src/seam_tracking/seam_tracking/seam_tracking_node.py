import rclpy
import json
from rclpy.node import Node

from shared_interfaces.msg import ModbusCoord
from sensor_msgs.msg import PointCloud2
from share_interfaces.srv import Code
from share_interfaces.srv import CodeList

from . import ros2_numpy as rnp

def Calculate(x, y, num, delta):
    index = y.index(max(y))
    if index - num < 0 and index + num > len(y):
        return None, None
    b1, m1 = polyfit(x[index - num : index - delta], y[index - num : index - delta], 1)
    b2, m2 = polyfit(x[index + delta : index + num], y[index + delta : index + num], 1)
    try:
        px = (b2 - b1) / (m2 - m1)
        py = px * m1 + b1
    except:
        return None, None
    else:
        return px, py

def Convolve(x, y, k = 10, t = 5):
    dy = [0.] * len(y)
    for i in range(k, len(y) - k):
        for j in range(-k, k + 1):
            dy[i] += y[i] - y[i + j]
    v = min(dy)
    return dy.index(v) if v < -t else None

def Segment(x, y, dx, dy, count = 1):
    segX, segY = [], []
    # if not x or not y:
    #     return segX, segY
    
    L = list(zip(x, y))

    while(L):
        (X, Y), *L = L
        tmpX, tmpY = [X], [Y]
        while(L):
            if abs(X - L[0][0]) > dx or abs(Y - L[0][1]) > dy:
                break
            else:
                (X, Y), *L = L
                tmpX.append(X)
                tmpY.append(Y)
        if len(tmpX) > count:
            segX.append(tmpX)
            segY.append(tmpY)
    
    return segX, segY

def Candidates(segX, segY):
    cx, cy = [], []
    for lx, ly in zip(segX, segY):
        cx.extend([lx[0], lx[-1]])
        cy.extend([ly[0], ly[-1]])

    return cx, cy

def Pick(cx, cy, index):
    if index < len(cx) or index < 0:
        return cx[index], cy[index]
    else:
        return None, None

def Exe(x, y, dx, dy, count = 1):
    segX, segY = Segment(x, y, dx, dy, count)
    return Candidates(segX, segY)

class SeamTracking(Node):


    def __init__(self):
        Node.__init__(self, 'seam_tracking_node')
        qos = rclpy.qos.qos_profile_sensor_data
        self.pub = self.create_publisher(ModbusCoord, '~/coord', 10)
        self.sub = self.create_subscription(PointCloud2, '~/pnts', self._cb, qos)
        self.srv_code_list = self.create_service(CodeList, '~/codeList', self._cb_code_list)
        self.srv_code = self.create_service(Code, '~/code', self._cb_code)
        self.get_logger().info('Initialized successfully')

    def __del__(self):
        self.get_logger().info('Destroyed successfully')

    def _cb_code_list(self, request, response):
        pass

    def _cb_code(self, request, response):
        pass

    def _cb(self, msg):
        ret = ModbusCoord()
        ret.valid = False
        
        if msg.height * msg.width == 0:
            self.pub.publish(ret)
            return
        
        data = rnp.numpify(msg)
        # y, z = Calculate(data['y'], data['z'], 110, 10)
        cy, cz = Exe(data['y'], data['z'], 2, 5, 50)
        y, z = Pick(cy, cz, 0)
        # i = Convolve(data['y'] * 100., data['z'] * 100.)
        if y:
            ret.valid, ret.x, ret.y, ret.z = True, 0., float(y), float(z)
            self.pub.publish(ret)
        else:
            self.pub.publish(ret)

    def _load_codes(self):
        try:
            with open('codes.json', 'r') as f:
                self.codes = json.load(f)
        except FileNotFoundError:
            self.codes = {}

    def _save_codes(self):
        with open('codes.json', 'w') as f:
            json.dump(self.codes, f)

def main(args=None):
    rclpy.init(args=args)

    sm = SeamTracking()

    try:
        rclpy.spin(sm)
    except KeyboardInterrupt:
        pass
    finally:
        # Destroy the node explicitly
        # (optional - otherwise it will be done automatically
        # when the garbage collector destroys the node object)
        sm.destroy_node()
        rclpy.shutdown()

if __name__ == '__main__':
    main()
