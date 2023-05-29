SET SQL_MODE = "NO_AUTO_VALUE_ON_ZERO";
SET AUTOCOMMIT = 0;
START TRANSACTION;
SET time_zone = "+00:00";

CREATE TABLE `tw_Accounts` (
    `ID` INTEGER PRIMARY KEY AUTO_INCREMENT,
    `Username` TEXT NOT NULL,
    `Password` TEXT NOT NULL,
    `Language` TEXT NOT NULL,
    `Vip` INTEGER NOT NULL DEFAULT '0',
    `Po` INTEGER NOT NULL DEFAULT '0',
    `Xiuwei` INTEGER NOT NULL DEFAULT '1'
);

CREATE TABLE `tw_ItemList` (
    `ID` INTEGER PRIMARY KEY AUTO_INCREMENT,
    `ItemType` INTEGER NOT NULL DEFAULT '1',
    `Name` TEXT NOT NULL,
    `Desc` TEXT NOT NULL,
    `Maxium` INTEGER NOT NULL DEFAULT '-1'
);

INSERT INTO `tw_ItemList` (`ID`, `ItemType`, `Name`, `Desc`, `Maxium`) VALUES
    (1, -1, '道铃', '- 摇响即可使用3个月寿命召唤游姥爷', 1),
    (2, -1, '劣质的人丹', '- 用人的魂魄炼成，服下后短暂增强力量', -1),
    (3, -1, '平凡的人丹', '- 用人的魂魄炼成，服下后短暂增强力量', -1),
    (4, -1, '中等的人丹', '- 用人的魂魄炼成，服下后短暂增强力量', -1);