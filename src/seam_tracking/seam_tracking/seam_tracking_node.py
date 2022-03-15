import rclpy

from rclpy.node import Node
from shared_interfaces.msg import ModbusCoord
from sensor_msgs.msg import PointCloud2
from std_srvs.srv import Trigger
from shared_interfaces.srv import CountCodes
from shared_interfaces.srv import GetCode
from shared_interfaces.srv import SetCode

# from shared_interfaces.srv import ListCodes
from shared_interfaces.srv import SelectCode
from .codes import Codes
import ros2_numpy as rnp

# def Calculate(x, y, num, delta):
#     index = y.index(max(y))
#     if index - num < 0 and index + num > len(y):
#         return None, None
#     b1, m1 = polyfit(x[index - num : index - delta], y[index - num : index - delta], 1)
#     b2, m2 = polyfit(x[index + delta : index + num], y[index + delta : index + num], 1)
#     try:
#         px = (b2 - b1) / (m2 - m1)
#         py = px * m1 + b1
#     except:
#         return None, None
#     else:
#         return px, py

# def Convolve(x, y, k = 10, t = 5):
#     dy = [0.] * len(y)
#     for i in range(k, len(y) - k):
#         for j in range(-k, k + 1):
#             dy[i] += y[i] - y[i + j]
#     v = min(dy)
#     return dy.index(v) if v < -t else None

# def Segment(x, y, dx, dy, count = 1):
#     segX, segY = [], []
#     # if not x or not y:
#     #     return segX, segY
    
#     L = list(zip(x, y))

#     while(L):
#         (X, Y), *L = L
#         tmpX, tmpY = [X], [Y]
#         while(L):
#             if abs(X - L[0][0]) > dx or abs(Y - L[0][1]) > dy:
#                 break
#             else:
#                 (X, Y), *L = L
#                 tmpX.append(X)
#                 tmpY.append(Y)
#         if len(tmpX) > count:
#             segX.append(tmpX)
#             segY.append(tmpY)
    
#     return segX, segY

# def Candidates(segX, segY):
#     cx, cy = [], []
#     for lx, ly in zip(segX, segY):
#         cx.extend([lx[0], lx[-1]])
#         cy.extend([ly[0], ly[-1]])

#     return cx, cy

# def Pick(cx, cy, index):
#     if index < len(cx) or index < 0:
#         return cx[index], cy[index]
#     else:
#         return None, None

# def Exe(x, y, dx, dy, count = 1):
#     segX, segY = Segment(x, y, dx, dy, count)
#     return Candidates(segX, segY)


class SeamTracking(Node):


    def __init__(self):
        Node.__init__(self, 'seam_tracking_node')
        self.pnts = [(False, 0., 0.) for i in range(3)]
        qos = rclpy.qos.qos_profile_sensor_data
        self.pub = self.create_publisher(ModbusCoord, '~/coord', 10)
        self.sub = self.create_subscription(PointCloud2, '~/pnts', self._cb, qos)
        self.srv_count_codes = self.create_service(CountCodes, '~/count_codes', self._cb_count_codes)
        self.srv_get_code = self.create_service(GetCode, '~/get_code', self._cb_get_code)
        self.srv_set_code = self.create_service(SetCode, '~/set_code', self._cb_set_code)
        # self.srv_list_codes = self.create_service(ListCodes, '~/list_codes', self._cb_list_codes)
        # self.srv_set_code = self.create_service(SetCode, '~/set_code', self._cb_set_code)
        # self.srv_save_codes = self.create_service(Trigger, '~/save_codes', self._cb_save_codes)
        self.srv_select_code = self.create_service(SelectCode, '~/select_code', self._cb_select_code)
        self.codes = Codes()

        self.get_logger().info('Initialized successfully')

    def __del__(self):
        self.get_logger().info('Destroyed successfully')

    def _append_pnts(self, ret, *, dx = 1., dy = 1.):
        self.pnts.append(ret)
        self.pnts.pop(0)
        for i in range(2):
            v0, x0, y0 = self.pnts[i]
            v1, x1, y1 = self.pnts[i + 1]
            if v0 == False or v1 == False or abs(x1 - x0) > dx or abs(y1 - y0) > dy:
                return False, 0., 0.
        return self.pnts[2]

    def _cb_count_codes(self, request, response):
        response.num = len(self.codes)
        return response

    def _cb_get_code(self, request, response):
        response.code = self.codes[request.index]
        return response

    def _cb_set_code(self, request, response):
        self.codes[request.index] = request.code
        response.success = True
        return response

    # def _cb_list_codes(self, request, response):
    #     with self.lock:
    #         response.codes = self.codes
    #     response.success = True
    #     response.message = 'List codes successfully'

    # def _cb_set_code(self, request, response):
    #     with self.lock:
    #         if request.index < len(self.codes):
    #             self.codes[request.index] = request.code
    #             response.success = True
    #             response.message = 'Set code successfully'
    #         else:
    #             response.success = False
    #             response.message = 'Index out of range'

    # def _cb_save_codes(self, request, response):
    #     with self.lock, open('codes.json', 'w') as f:
    #         json.dump(self.codes, f)
    #     response.success = True
    #     response.message = 'Save codes successfully'

    def _cb_select_code(self, request, response):
        if 0 <= request.index < len(self.codes):
            self.codes.reload(request.index)
            response.success = True
            response.message = 'Select code successfully'
        else:
            response.success = False
            response.message = 'Index out of range'
        return response

    def _cb(self, msg):
        ret = ModbusCoord()
        ret.x = 0.
        ret.valid = False

        if msg.height * msg.width == 0:
            self.pub.publish(ret)
            return

        data = rnp.numpify(msg)
        try:
            pnt = self.codes(data['y'], data['z'])
            ret.valid, ret.y, ret.z = self._append_pnts(pnt, dx = 0.5, dy = 0.5)
        except Exception as e:
            self.get_logger().warn(str(e))
        finally:
            self.pub.publish(ret)
        # y, z = Calculate(data['y'], data['z'], 110, 10)
        # cy, cz = Exe(data['y'], data['z'], 2, 5, 50)
        # y, z = Pick(cy, cz, 0)
        # i = Convolve(data['y'] * 100., data['z'] * 100.)
        # if y:
        #     ret.valid, ret.x, ret.y, ret.z = True, 0., float(y), float(z)
        #     self.pub.publish(ret)
        # else:
        #     self.pub.publish(ret)


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
