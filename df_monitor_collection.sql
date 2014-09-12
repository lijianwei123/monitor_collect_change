/*
Navicat MySQL Data Transfer

Source Server         : 168.192.122.30
Source Server Version : 50509
Source Host           : 168.192.122.30:3306
Source Database       : df_mixed

Target Server Type    : MYSQL
Target Server Version : 50509
File Encoding         : 65001

Date: 2014-09-12 18:44:00
*/

SET FOREIGN_KEY_CHECKS=0;

-- ----------------------------
-- Table structure for `df_monitor_collection`
-- ----------------------------
DROP TABLE IF EXISTS `df_monitor_collection`;
CREATE TABLE `df_monitor_collection` (
  `id` int(11) unsigned NOT NULL AUTO_INCREMENT,
  `url` varchar(200) NOT NULL,
  `pattern` varchar(200) NOT NULL,
  `match` tinyint(1) NOT NULL DEFAULT '0' COMMENT '是否匹配',
  `add_time` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP,
  `modify_time` datetime NOT NULL,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB AUTO_INCREMENT=80 DEFAULT CHARSET=utf8 COMMENT='监控采集页面变动';

-- ----------------------------
-- Records of df_monitor_collection
-- ----------------------------
INSERT INTO `df_monitor_collection` VALUES ('4', 'http://hotel.elong.com/beihai/02103235/', '<h1>\\s*?<i\\s*?id=\\\"hotelGrade\\\"\\s*?class=\\\"\\\"></i>\\s*?(\\S+)\\s*?</h1>', '1', '2014-09-02 19:41:43', '2014-09-12 17:37:57');
