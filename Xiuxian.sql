SET SQL_MODE = "NO_AUTO_VALUE_ON_ZERO";
SET AUTOCOMMIT = 0;
START TRANSACTION;
SET time_zone = "+00:00";

DROP DATABASE xiuxian;
CREATE DATABASE xiuxian;
USE xiuxian;

CREATE TABLE `tw_Accounts` (
    `ID` int(11) NOT NULL,
    `Username` TEXT NOT NULL,
    `Password` TEXT NOT NULL,
    `Language` TEXT NOT NULL,
    `Vip` INTEGER NOT NULL DEFAULT '0',
    `Po` INTEGER NOT NULL DEFAULT '0',
    `Xiuwei` INTEGER NOT NULL DEFAULT '1',
    `Tizhi` INTEGER NOT NULL DEFAULT '0',
    `Yuansu` TEXT NOT NULL,
    `ZongMen` INTEGER NOT NULL DEFAULT '0'
);

CREATE TABLE `tw_uItemList` (
    `UserID` INTEGER NOT NULL DEFAULT '-1',
    `ItemID` INTEGER NOT NULL DEFAULT '-1',
    `Num` INTEGER NOT NULL DEFAULT '0',
    `Extra` TEXT NOT NULL
);

CREATE TABLE `tw_ItemList` (
    `ID` int(11) NOT NULL,
    `ItemType` INTEGER NOT NULL DEFAULT '-1',
    `Name` TEXT NOT NULL,
    `Desc` TEXT NOT NULL,
    `Maxium` INTEGER NOT NULL DEFAULT '-1'
);

INSERT INTO `tw_ItemList` (`ID`, `ItemType`, `Name`, `Desc`, `Maxium`) VALUES 
(1, 0, '道铃', '- 摇响即可使用3个月寿命召唤游姥爷', 1),
(2, 2, '劣质的人丹', '- 用人的十情八苦炼成，服下后短暂增强力量，但移动、攻击速度减弱', -1),
(3, 2, '平凡的人丹', '- 用人的十情八苦炼成，服下后短暂增强力量和速度，但攻击速度减弱', -1),
(4, 2, '中等的人丹', '- 用人的十情八苦炼成，服下后增强力量', -1),
(5, 2, '高等的人丹', '- 用人的十情八苦炼成，服下后增强力量和速度并提高修为(强行突破一级)', -1),
(6, 6, '空白的麻将牌', '- 一个空白的麻将牌，似乎等待着他的主人在上面刻下烙印', -1),
(7, 0, '奇怪的石板', '- 厚重的石板，上面刻着劝人向善的佛经', -1),
(8, 1, '大千录', '- （攻击系）一本功法，用图画记载了各种折磨人的方式，以及一个可怕的密术：《苍蜣登阶》', -1),
(9, 0, '铜钱面罩', '- 一个面罩，能掩盖你的气息，心素必备', 1),
(10, 1, '火袄真经', '- （治疗系）一本功法，能治疗外伤与内伤，但会着火', -1);

CREATE TABLE `tw_ZongMenList` (
    `ID` int(11) NOT NULL,
    `Name` TEXT NOT NULL,
    `Desc` TEXT NOT NULL,
    `ZongZhu` INTEGER NOT NULL
);

INSERT INTO `tw_ZongMenList` (`ID`, `Name`, `Desc`, `ZongZhu`) VALUES
(1, '邪祟', '邪祟，好你个邪祟！', -1),
(2, '袄景教', '（祭拜痛苦）向古神巴虺献出痛苦... 夺走她的力量！', -1),
(3, '白莲教', '（祭拜无生老母）无生老母，真空家乡', -1),
(4, '正德寺', '（色孽菩萨）我佛慈悲，道友所见皆为虚幻（血肉增殖 无穷色欲）', -1),
(5, '安慈庵', '玩家不可加入该门派', -1),
(6, '出马仙', '（被仙家奴隶）永远成为仙家的奴隶...', -1),
(7, '傩戏', '玩家不可加入该门派', -1),
(8, '兵家', '（祭拜将相首）我他马杀杀杀杀杀杀杀杀杀杀杀杀杀杀杀杀杀杀杀杀杀杀杀杀杀杀杀杀杀杀杀杀杀杀杀杀杀杀杀杀杀杀杀杀杀杀杀', -1),
(9, '舞狮宫', '玩家不可加入该门派', -1),
(10, '罗教', '无为解脱，三教归一', -1),
(11, '中阴庙', '玩家不可加入该门派', -1),
(12, '法教', '（祭拜于儿神）', -1),
(13, '方仙道', '精神专一，动合无形，赡足万物', -1),
(14, '清风观', '-= 半 ⚠ 仙 =-', -1),
(15, '坐忘道', '（骗子）何谓坐忘道? 堕其肢体，黜遁聪者，离形去知，同于大通，此谓坐忘。', -1);

ALTER TABLE `tw_Accounts`
  ADD PRIMARY KEY (`ID`);

ALTER TABLE `tw_ItemList`
  ADD PRIMARY KEY (`ID`);

ALTER TABLE `tw_ZongMenList`
  ADD PRIMARY KEY (`ID`);

ALTER TABLE `tw_Accounts`
  MODIFY `ID` int(11) NOT NULL AUTO_INCREMENT, AUTO_INCREMENT=1;