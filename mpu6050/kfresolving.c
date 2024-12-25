#include "kfresolving.h"
#include "main.h"
#include "mpu6050.h"
#include "math.h"
#include "usart.h"

double pitch_g=0 , pitch_a=0 , roll_g=0 , roll_a=0 , pitch_g_1=0 , pitch_a_1=0 , roll_g_1=0 , roll_a_1=0 ,pitch_g_2=0,roll_g_2=0;
short ax_2=0,ay_2=0,az_2=0,gx_2=0,gy_2=0,gz_2=0;
short ax_3,ay_3,az_3,gx_3,gy_3,gz_3=0;
double P[100];
double P_P[2][2]={{1,0},{0,1}};  
double K_K[2][2]={{0,0},{0,0}};
double now,last_time,dt;

int state=0;

uint8_t message[2]={0,9};
extern int step;


void kf()
{
	MPU6050_Read_Accel();
	MPU6050_Read_Gyro();
	

	//将加速度得出来的角度作为观测值
	roll_a = atan(Mpu6050_Data.Accel_Y/Mpu6050_Data.Accel_Z)*180/3.14;
	pitch_a = -atan(Mpu6050_Data.Accel_X/sqrt(Mpu6050_Data.Accel_Y*Mpu6050_Data.Accel_Y+Mpu6050_Data.Accel_Z*Mpu6050_Data.Accel_Z))*180/3.14;

	
	dt = 0.001;
	//先验估计
  roll_g = roll_g_1 + (Mpu6050_Data.Gyro_X + ((sin(pitch_g_1)*sin(roll_g_1))/cos(pitch_g_1))*Mpu6050_Data.Gyro_Y + (cos(roll_g_1)*sin(pitch_g_1))/cos(pitch_g_1)*Mpu6050_Data.Gyro_Z)*dt;//
	pitch_g = pitch_g_1 + (cos(roll_g_1)*Mpu6050_Data.Gyro_Y - sin(roll_g_1)*Mpu6050_Data.Gyro_Z)*dt ;
	
	//先验协方差
	
	P_P[0][0] = P_P[0][0]+0.0025;
	P_P[0][1] = 0;
	P_P[1][0] = 0;
	P_P[1][1] = P_P[1][1]+0.0025;
	
	//卡尔曼增益
	
	K_K[0][0] = P_P[0][0]/(P_P[0][0]+ 0.3);
	K_K[0][1] = 0;
	K_K[1][0] = 0;
	K_K[1][1] = P_P[1][1]/(P_P[1][1] + 0.3);
	
	
	//最优解
	roll_g = roll_g + K_K[0][0]*(roll_a - roll_g);
	pitch_g = pitch_g + K_K[1][1]*(pitch_a - pitch_g); 
	
	//计步
	if((pitch_g < -30 ) && state==0)state = 1;
	else if(pitch_g > -30 && pitch_g < 30 && state==1)state = 2;
	else if(( pitch_g > 30) && state==2)state = 3;
	
	if(state == 3)
	{
		step++;
		HAL_UART_Transmit(&huart2,message,2,HAL_MAX_DELAY);
		state = 0;
	}
		
	//更新协方差
	P_P[0][0] =  (1-K_K[0][0])*P_P[0][0];
  P_P[0][1] = 0;
	P_P[1][0] = 0;
	P_P[1][1] = (1-K_K[1][1])*P_P[1][1];

	
	pitch_g_1 = pitch_g;
	roll_g_1 = roll_g;
	
	
}

//float pitch_kalman; 	 	//pitch滤波后数据
//float roll_kalman; 		 	//roll滤波后数据


////卡尔曼参数		
//static float Q_angle = 0.001;		//角度数据置信度，角度噪声的协方差
//static float Q_gyro  = 0.003;		//角速度数据置信度，角速度噪声的协方差  
//static float R_angle = 0.5;		//加速度计测量噪声的协方差
//static float dt      = 0.5;		//采样周期即计算任务周期10ms

//void Kalman_Cal_Pitch(float acc,float gyro) //卡尔曼滤波pitch轴计算		
//{
//	static float Q_bias;	//Q_bias:陀螺仪的偏差
//	static float K_0, K_1;	//卡尔曼增益  K_0:用于计算最优估计值  K_1:用于计算最优估计值的偏差 t_0/1:中间变量
//	static float PP[2][2] = { { 1, 0 },{ 0, 1 } };//过程协方差矩阵P，初始值为单位阵

//	/*
//		卡尔曼滤波的使用步骤
//		(1) 选择状态量、观测量
//		(2) 构建方程
//		(3) 初始化参数
//		(4) 带入公式迭代
//		(5) 调节超参数P、Q
//	*/
//	/*
//	X(k)：k时刻系统状态				Z(k)：k时刻测量值
//	U(k)：k时刻对系统控制量		H：测量系统参数
//																					 方差
//	A/F：状态转移矩阵					W(k)：过程噪声 ----> Q
//																					 方差	
//	B：控制矩阵								V(k)：测量噪声 ----> R
//	
//										离散控制系统
//	系统描述：X(k|k-1) = AX(k-1|k-1) + BU(k) + (W(k))
//	测量值：Z(k) = HX(k) + V(k)
//	*/
//	/*
//	1. 先验估计
//* * *公式1：X(k|k-1) = AX(k-1|k-1) + BU(k) + (W(k))

//		X = (Angle,Q_bias)
//		A(1,1) = 1,A(1,2) = -dt
//		A(2,1) = 0,A(2,2) = 1
//		注：上下连“[”代表矩阵
//		预测当前角度值：
//		[ angle ] 	[1 -dt][ angle ]	 [dt]
//		[ Q_bias] = [0  1 ][ Q_bias] + [ 0] * newGyro(加速度计测量值)
//		故
//		angle = angle - Q_bias*dt + newGyro * dt
//		Q_bias = Q_bias
//	*/
//	pitch_kalman += (gyro - Q_bias) * dt; //状态方程,角度值等于上次最优角度加角速度减零漂后积分
//	
//	/*
//	2. 预测协方差矩阵
//* * *公式2：P(k|k-1)=AP(k-1|k-1)A^T + Q 

//		由先验估计有系统参数
//				[1 -dt]
//		A = [0  1 ]
//		
//		系统过程协方差噪声Q定义：
//		| cov(angle,angle)  cov(Q_bias,angle) |
//		|	cov(angle,Q_bias) cov(Q_bias,Q_bias)|
//			 角度噪声和角速度漂移噪声相互独立
//		| D( angle )  			0 	 |
//	= |			0 			D( Q_bias )|
//		又Q_angle和Q_bias的方差为常数，
//		可由经验或计算得出
//		
//		令D( angle )  = Q_angle 
//			D( Q_bias ) = Q_gyro 
//			
//		设上一次预测协方差矩阵为P(k-1)
//											|a(k-1)  b(k-1)|
//											|c(k-1)  d(k-1)|
//		本次预测协方差矩阵P(k)
//											|a(k)  b(k)|
//		                  |c(k)  d(k)|
//		由公式2：P(k|k-1)=AP(k-1|k-1)A^T + Q 得
//		|a(k)  b(k)|		|1 -dt|	|a(k-1) b(k-1)| |1 	 0|		| D( angle )  			0 	 |
//		|c(k)  d(k)| =  |0  1 | |c(k-1) d(k-1)| |-dt 1| + |			0 			D( Q_bias )|
//	
//		进一步得
//		|a(k)  b(k)|		|a(k-1) - [c(k-1) + b(k-1)]*dt + d(dt)^2		b(k-1) - d(k-1)*dt|		| D( angle )  			0 	 |
//	  |c(k)  d(k)| =  |				c(k-1) - d(k-1)*dt													d(k-1)		| + |			0 			D( Q_bias )|
//		
//		由于dt^2太小，故dt^2省略
//	*/
//	
//	PP[0][0] = PP[0][0] + Q_angle - (PP[0][1] + PP[1][0])*dt;
//	PP[0][1] = PP[0][1] - PP[1][1]*dt;
//	PP[1][0] = PP[1][0] - PP[1][1]*dt;
//	PP[1][1] = PP[1][1] + Q_gyro;
//	
//	/*
//		3. 建立测量方程
//			系统测量方程 Z(k) = HX(k) + V(k)
//			系统测量系数 H = [1, 0]
//			因为陀螺仪输出自带噪声
//			所以
//			measure = newAngle
//	*/
//	
//	/*
//		4. 计算卡尔曼增益
//* * *公式3：Kg(k)= P(k|k-1)H^T/(HP(k|k-1)H^T+R)
//				Kg = (K_0,K_1) 对应angle,Q_bias增益
//				H = (1,0)
//	*/
//	K_0 = PP[0][0] / (PP[0][0] + R_angle);
//	K_1 = PP[1][0] / (PP[0][0] + R_angle);

//	/*
//		5. 计算当前最优化估计值
//* * *公式4：X(k|k) = X(k|k-1) + kg(k)[z(k) - HX(k|k-1)]
//		angle = angle + K_0*(newAngle - angle)
//		Q_bias = Q_bias + K_1*(newAngle - angle)
//	*/
//		
//	pitch_kalman = pitch_kalman + K_0 * (acc - pitch_kalman);
//	Q_bias = Q_bias + K_1 * (acc - pitch_kalman);

//	/*
//		6. 更新协方差矩阵
//* * *公式5：P(k|k)=[I-Kg(k)H]P(k|k-1)
//	*/
//	PP[0][0] = PP[0][0] - K_0 * PP[0][0];
//	PP[0][1] = PP[0][1] - K_0 * PP[0][1];
//	PP[1][0] = PP[1][0] - K_1 * PP[0][0];
//	PP[1][1] = PP[1][1] - K_1 * PP[0][1];

//}

//void Kalman_Cal_Roll(float acc,float gyro) //卡尔曼滤波roll轴计算				
//{
//	static float Q_bias;	//Q_bias:陀螺仪的偏差  Angle_err:角度偏量 
//	static float K_0, K_1;	//卡尔曼增益  K_0:用于计算最优估计值  K_1:用于计算最优估计值的偏差 t_0/1:中间变量
//	static float PP[2][2] = { { 1, 0 },{ 0, 1 } };//过程协方差矩阵P，初始值为单位阵
//	roll_kalman += (gyro - Q_bias) * dt; //状态方程,角度值等于上次最优角度加角速度减零漂后积分
//	PP[0][0] = PP[0][0] + Q_angle - (PP[0][1] + PP[1][0])*dt;
//	PP[0][1] = PP[0][1] - PP[1][1]*dt;
//	PP[1][0] = PP[1][0] - PP[1][1]*dt;
//	PP[1][1] = PP[1][1] + Q_gyro;
//	K_0 = PP[0][0] / (PP[0][0] + R_angle);
//	K_1 = PP[1][0] / (PP[0][0] + R_angle);
//	roll_kalman = roll_kalman + K_0 * (acc - roll_kalman);
//	Q_bias = Q_bias + K_1 * (acc - roll_kalman);
//	PP[0][0] = PP[0][0] - K_0 * PP[0][0];
//	PP[0][1] = PP[0][1] - K_0 * PP[0][1];
//	PP[1][0] = PP[1][0] - K_1 * PP[0][0];
//	PP[1][1] = PP[1][1] - K_1 * PP[0][1];
//}